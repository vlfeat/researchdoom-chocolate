# ResearchDoom: Fixing an out-of-bound textel read

We found that RDM fails to exactly reproduce all generated pixels upon re-running the recoring, as the color of a tiny fraction of pixels is undefined.


## Preliminaries

Relevant to debugging this issue is the definition of fixed precision multiplication and division in Doom:

```c
// assuming x86-64 or amd64

#define FRACBITS 16
typedef int32_t fixed_t;

fixed_t FixedMul(fixed_t a, fixed_t b)
{
return ((int64_t) a * (int64_t) b) >> FRACBITS;
}

fixed_t FixedDiv(fixed_t a, fixed_t b)
{
if ((abs(a) >> 14) >= abs(b))
{
    return (a^b) < 0 ? INT_MIN : INT_MAX;
}
else
{
    int64_t result;
    result = ((int64_t) a << FRACBITS) / b;
    return (fixed_t) result;
}
}
```

If $a$ and $b$ are integers in the range $-2^{15}$ to $2^{15}-1$, and if $F = 2^{16}$, then on x86-64 this implementation satisfies
$$
\operatorname{FixedMul}(a,b)=\left\lfloor \frac{a b}{F}\right\rfloor
$$
We also have
$$
\operatorname{FixedDiv}(a,b)=\operatorname{trunc} \left( \frac{a F}{b} \right),
$$
at least if $a$ and $b$ as to avoid the saturation branch.


## Accessing texture elements

Within a post (a vertical contiguous non-transparent texture chunk), texels start to read at

```c
frac0 = vis->texturemid - (column->topdelta << FRACBITS) + (dc_yl - centery) * dc_iscale
dc_iscale = FixedDiv(FRACUNIT, spryscale)
```

where the smallest value of `dc_yl` is determined by the code:

```c
dc_yl = (topscreen + FRACUNIT - 1) >> FRACBITS
topscreen = sprtopscreen + spryscale * topdelta
sprtopscreen = centery * F - FixedMul(texturemid, spryscale)
```

In formulas, we have that the minimum value of `dc_yl` is given by:
$$
\mathtt{dc\_yl} =
\left\lceil
\frac{
\mathtt{centery} \cdot F -
\left\lfloor
\frac{\mathtt{texturemid} \cdot \mathtt{spryscale}}{F}
\right\rfloor
+
\mathtt{spryscale} \cdot \mathtt{topdelta}
}
{
F
}
\right\rceil
$$
If we assume that the integer $\mathtt{spryscale}$ is greater than 4 (to avoid triggering the saturation in FixedDiv), we have:
$$
\mathtt{dc\_iscale}
=
\left\lfloor
\frac{F^2}{\mathtt{spryscale}}
\right\rfloor
$$
so that:
$$
\begin{align*}
\mathtt{frac0} = &
\mathtt{texturemid} -
\mathtt{topdelta} \cdot F +
(\mathtt{dc\_yl} - \mathtt{centery})
\cdot
\left\lfloor
\frac{F^2}{\mathtt{spryscale}}
\right\rfloor
\\
&=
\mathtt{texturemid} -
\mathtt{topdelta} \cdot F -
K
\cdot
\left\lfloor
\frac{F^2}{\mathtt{spryscale}}
\right\rfloor
\end{align*}
$$
where $K$ simplifies to:
$$
K = \mathtt{centery} - \mathtt{dc\_yl}
= \left\lfloor
\frac{
\left\lfloor
\frac{\mathtt{texturemid} \cdot \mathtt{spryscale}}{F}
\right\rfloor
-
\mathtt{topdelta} \cdot \mathtt{spryscale}
}
{
F
}
\right\rfloor
$$
We can now bound `frac0` by noting that:
$$
\begin{align*}
\mathtt{texturemid} -
\mathtt{topdelta} \cdot F
&=
\frac{
\frac{\mathtt{texturemid} \cdot \mathtt{spryscale}}{F}
-
\mathtt{topdelta} \cdot \mathtt{spryscale}
}
{
F
}
\cdot
\frac{F^2}{\mathtt{spryscale}}
\\
&\geq
\left\lfloor
\frac{
\left\lfloor
\frac{\mathtt{texturemid} \cdot \mathtt{spryscale}}{F}
\right\rfloor
-
\mathtt{topdelta} \cdot \mathtt{spryscale}
}
{
F
}
\right\rfloor
\cdot
\frac{F^2}{\mathtt{spryscale}}
\\
&\geq
K \left(
\left\lfloor
\frac{F^2}{\mathtt{spryscale}}
\right\rfloor
+ 1_{K<0}
\right)
\end{align*}
$$
Thus, we see that, with the original definition, `frac0` is indeed non-negative if $K \geq 0$.
When $K<0$, however, we need to modify the rounding of $F^2 / \mathtt{spryscale}$ by adding 1 to it.
Then, if we were to instead set

```c
// ResearchDoom +(centery  < dc_yl)) fix to avoid very rare overflows.
frac = dc_texturemid + (dc_yl-centery)*(fracstep + (centery  < dc_yl));
```

we guarantee that $\mathtt{frac0} \geq 0$ for the unclipped value of `dc_yl` above.

Next, we look at the largest possible accessed texel. For a given runtime value of `dc_yl` (which can be the minimum given above or some value larger than it in case of clipping), the largest texel coordinate is given by:

```
frac1 = frac0 + count * dc_iscale
count = dc_yh - dc_yl
dc_yh = (bottomscreen-1)>>FRACBITS
bottomscreen = topscreen + spryscale * length
```

Assuming here that there is no bottom clipping, we thus have:
$$
\begin{align*}
\mathtt{frac1} &=
\mathtt{texturemid} -
\mathtt{topdelta} \cdot F +
(\mathtt{dc\_yl} - \mathtt{centery})
\left(
\left\lfloor
\frac{F^2}{\mathtt{spryscale}}
\right\rfloor
+ 1_{\mathtt{dc\_yl} > \mathtt{centery}}
\right)
+
(\mathtt{dc\_yh} - \mathtt{dc\_yl})
\left\lfloor
\frac{F^2}{\mathtt{spryscale}}
\right\rfloor
\\
&=
\mathtt{texturemid} -
\mathtt{topdelta} \cdot F +
[\mathtt{dc\_yl} - \mathtt{centery}]_+
+
(\mathtt{dc\_yh} - \mathtt{centery})
\left\lfloor
\frac{F^2}{\mathtt{spryscale}}
\right\rfloor
\\
&=
\mathtt{texturemid} -
\mathtt{topdelta} \cdot F +
[\mathtt{dc\_yl} - \mathtt{centery}]_+
+
Q
\left\lfloor
\frac{F^2}{\mathtt{spryscale}}
\right\rfloor
\end{align*}
$$
where
$$
Q
=
\mathtt{dc\_yh} - \mathtt{centery}
=
\left\lfloor
\frac{
    \mathtt{spryscale}\cdot(\mathtt{topdelta}+\mathtt{length})
-
\left\lfloor
\frac{\mathtt{texturemid}\cdot \mathtt{spryscale}}{F}
\right\rfloor
-1
}{
F
}
\right\rfloor
$$
We can bound $Q$ as follows:
$$
Q \leq
H =
\frac{
\mathtt{spryscale}\cdot(\mathtt{topdelta}+\mathtt{length})
-\frac{\mathtt{texturemid}\cdot \mathtt{spryscale}}{F}
}{
F
}
$$

Hence, we can write now again:
$$
\begin{align*}
\mathtt{frac1} &\leq
\mathtt{texturemid} -
\mathtt{topdelta} \cdot F +
[\mathtt{dc\_yl} - \mathtt{centery}]_+
+
H
\left(
\frac{F^2}{\mathtt{spryscale}}
-
1_{H < 0}
\right)
\\
&=
[\mathtt{dc\_yl} - \mathtt{centery}]_+ +
\mathtt{length} \cdot F
-
H \cdot 1_{H < 0}
\end{align*}
$$

This fails to show that `frac1` less than $\mathtt{length} \cdot F$.
Using the fact that $Q \leq H$ and that $-Q \cdot 1_{Q <0} = [-Q]_+$, we get this other bound
$$
\mathtt{frac1} \leq 
[\mathtt{dc\_yl} - \mathtt{centery}]_+ +
\mathtt{length} \cdot F
- Q \cdot 1_{Q < 0}
=
\mathtt{length} \cdot F
+
[-K]_+
+
[-Q]_+
$$

Next, we thus look for a value $R$ such that
$$
\mathtt{frac1}
\leq
\mathtt{length} \cdot F
+
[-K]_+
+
[-Q]_+
-
(\mathtt{dc\_yh} - \mathtt{dc\_yl}) \cdot R
< L \cdot F
$$
so that we can subtract $R$ from  `dc_iscale` and correct the bound.
We can pick:
$$
R_{\min}
=
\left\lfloor
\frac{
\max\!\left( [-K]_+, [-Q]_+ \right)
}{
\texttt{count}
}
\right\rfloor
+ 1
$$

or

$$
R
=
\max\!\left(
0,\;
\left\lfloor
\frac{
\mathtt{frac1} - (\mathtt{length}\cdot F - 1) + \mathtt{count} - 1
}{
\mathtt{count}
}
\right\rfloor
\right).
$$

## Others

```shell
cmake -S. -Bbuild -DCMAKE_BUILD_TYPE=Debug -DENABLE_RDM_FIX=On ; cmake --build build --target clean ; cmake --build build --target chocolate-doom
cmake -S. -Bbuild -DCMAKE_BUILD_TYPE=Debug -DENABLE_RDM_FIX=Off ; cmake --build build --target clean ; cmake --build build --target chocolate-doom
```

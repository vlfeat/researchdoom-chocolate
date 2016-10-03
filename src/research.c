//
//  research.c
//  doom
//
//  Created by Andrea Vedaldi on 27/05/2016.
//  Copyright © 2016 Andrea Vedaldi. All rights reserved.
//

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdbool.h>

#include "png.h"

#include "research.h"

bool rdmIsRecording = false ;
rdmRecordingModeMask rdmRecordingMode = 0 ;
uint16_t * rdmDepthMapBuffer = NULL ;
uint8_t * rdmObjectMapBuffer = NULL ;
bool rdmHidePlayer = false ;
bool rdmSyncFrames = false ;

char rdmBaseName [1024] ;
size_t rdmLastRecordedTic = 0 ;
size_t rdmFrameHeight ;
size_t rdmFrameWidth ;
FILE * rdmLogFile = NULL ;

char const* researchObjectTypeNames [] = {
  "PLAYER",
  "POSSESSED",
  "SHOTGUY",
  "VILE",
  "FIRE",
  "UNDEAD",
  "TRACER",
  "SMOKE",
  "FATSO",
  "FATSHOT",
  "CHAINGUY",
  "TROOP",
  "SERGEANT",
  "SHADOWS",
  "HEAD",
  "BRUISER",
  "BRUISERSHOT",
  "KNIGHT",
  "SKULL",
  "SPIDER",
  "BABY",
  "CYBORG",
  "PAIN",
  "WOLFSS",
  "KEEN",
  "BOSSBRAIN",
  "BOSSSPIT",
  "BOSSTARGET",
  "SPAWNSHOT",
  "SPAWNFIRE",
  "BARREL",
  "TROOPSHOT",
  "HEADSHOT",
  "ROCKET",
  "PLASMA",
  "BFG",
  "ARACHPLAZ",
  "PUFF",
  "BLOOD",
  "TFOG",
  "IFOG",
  "TELEPORTMAN",
  "EXTRABFG",
  "MISC0",
  "MISC1",
  "MISC2",
  "MISC3",
  "MISC4",
  "MISC5",
  "MISC6",
  "MISC7",
  "MISC8",
  "MISC9",
  "MISC10",
  "MISC11",
  "MISC12",
  "INV",
  "MISC13",
  "INS",
  "MISC14",
  "MISC15",
  "MISC16",
  "MEGA",
  "CLIP",
  "MISC17",
  "MISC18",
  "MISC19",
  "MISC20",
  "MISC21",
  "MISC22",
  "MISC23",
  "MISC24",
  "MISC25",
  "CHAINGUN",
  "MISC26",
  "MISC27",
  "MISC28",
  "SHOTGUN",
  "SUPERSHOTGUN",
  "MISC29",
  "MISC30",
  "MISC31",
  "MISC32",
  "MISC33",
  "MISC34",
  "MISC35",
  "MISC36",
  "MISC37",
  "MISC38",
  "MISC39",
  "MISC40",
  "MISC41",
  "MISC42",
  "MISC43",
  "MISC44",
  "MISC45",
  "MISC46",
  "MISC47",
  "MISC48",
  "MISC49",
  "MISC50",
  "MISC51",
  "MISC52",
  "MISC53",
  "MISC54",
  "MISC55",
  "MISC56",
  "MISC57",
  "MISC58",
  "MISC59",
  "MISC60",
  "MISC61",
  "MISC62",
  "MISC63",
  "MISC64",
  "MISC65",
  "MISC66",
  "MISC67",
  "MISC68",
  "MISC69",
  "MISC70",
  "MISC71",
  "MISC72",
  "MISC73",
  "MISC74",
  "MISC75",
  "MISC76",
  "MISC77",
  "MISC78",
  "MISC79",
  "MISC80",
  "MISC81",
  "MISC82",
  "MISC83",
  "MISC84",
  "MISC85",
  "MISC86"};

// -------------------------------------------------------------------

void savePNG(const char * fileName, uint8_t const * pixels, size_t width, size_t height, uint8_t const * palette)
{
  png_structp png_ptr;
  png_infop info_ptr;
  png_colorp pal_ptr;
  FILE *file ;

  // Open file for writing (binary mode)
  file = fopen(fileName, "wb");
  if (file == NULL) {
    return ;
  }

  // Initialize write structure
  png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL); /* err_ptr, err_fn, warn_fn */
  if (!png_ptr)
  {
    fclose(file) ;
    return ;
  }

  // Initialize info structure
  info_ptr = png_create_info_struct(png_ptr);
  if (!info_ptr)
  {
    png_destroy_write_struct(&png_ptr, NULL);
    fclose(file) ;
    return ;
  }

  // Setup Exception handling
  if (setjmp(png_jmpbuf(png_ptr)))	/* catch errors */
  {
    png_destroy_write_struct(&png_ptr, &info_ptr);
    fclose(file) ;
    return ;
  }

  /* Setup file io */
  png_init_io(png_ptr, file);

  /* Set palette */
  pal_ptr = (png_colorp)malloc(256 * sizeof(png_color));
  for (size_t i = 0; i < 256 ; i++) {
    pal_ptr[i].red   = *palette++ ;
    pal_ptr[i].green = *palette++ ;
    pal_ptr[i].blue  = *palette++ ;
    palette++ ;
  }
  png_set_PLTE(png_ptr, info_ptr, pal_ptr, 256);
  free(pal_ptr);

  /* */
  png_set_IHDR(png_ptr, info_ptr, width, height, 8,
               PNG_COLOR_MASK_COLOR | PNG_COLOR_MASK_PALETTE,
               PNG_INTERLACE_NONE,
               PNG_COMPRESSION_TYPE_DEFAULT,
               PNG_FILTER_TYPE_DEFAULT);

  /* Write everything */
  png_write_info(png_ptr, info_ptr);
  for (size_t i = 0; i < height; i++) {
    png_write_row(png_ptr, (png_bytep)pixels + i * width);
  }
  png_write_end(png_ptr, info_ptr);

  /* Done */
  png_destroy_write_struct(&png_ptr, &info_ptr);
  fclose(file) ;
  return ;
}

void savePNG16(const char * fileName, uint16_t const * pixels, size_t width, size_t height)
{
  png_structp png_ptr;
  png_infop info_ptr;
  png_colorp pal_ptr;
  FILE *file ;

  // Open file for writing (binary mode)
  file = fopen(fileName, "wb");
  if (file == NULL) {
    return ;
  }

  // Initialize write structure
  png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL); /* err_ptr, err_fn, warn_fn */
  if (!png_ptr)
  {
    fclose(file) ;
    return ;
  }

  // Initialize info structure
  info_ptr = png_create_info_struct(png_ptr);
  if (!info_ptr)
  {
    png_destroy_write_struct(&png_ptr, NULL);
    fclose(file) ;
    return ;
  }

  // Setup Exception handling
  if (setjmp(png_jmpbuf(png_ptr)))	/* catch errors */
  {
    png_destroy_write_struct(&png_ptr, &info_ptr);
    fclose(file) ;
    return ;
  }

  /* Setup file io */
  png_init_io(png_ptr, file);

  /* */
  png_set_IHDR(png_ptr, info_ptr, width, height, 16,
               PNG_COLOR_TYPE_GRAY,
               PNG_INTERLACE_NONE,
               PNG_COMPRESSION_TYPE_DEFAULT,
               PNG_FILTER_TYPE_DEFAULT);

  /* Write everything */
  png_write_info(png_ptr, info_ptr);
  png_set_swap(png_ptr) ;
  for (size_t i = 0; i < height; i++) {
    png_write_row(png_ptr, (png_bytep)(pixels + i * width));
  }
  png_write_end(png_ptr, info_ptr);

  /* Done */
  png_destroy_write_struct(&png_ptr, &info_ptr);
  fclose(file) ;
  return ;
}

void savePNG24(const char * fileName, uint8_t const * pixels, size_t width, size_t height)
{
  png_structp png_ptr;
  png_infop info_ptr;
  png_colorp pal_ptr;
  FILE *file ;

  // Open file for writing (binary mode)
  file = fopen(fileName, "wb");
  if (file == NULL) {
    return ;
  }

  // Initialize write structure
  png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL); /* err_ptr, err_fn, warn_fn */
  if (!png_ptr)
  {
    fclose(file) ;
    return ;
  }

  // Initialize info structure
  info_ptr = png_create_info_struct(png_ptr);
  if (!info_ptr)
  {
    png_destroy_write_struct(&png_ptr, NULL);
    fclose(file) ;
    return ;
  }

  // Setup Exception handling
  if (setjmp(png_jmpbuf(png_ptr)))	/* catch errors */
  {
    png_destroy_write_struct(&png_ptr, &info_ptr);
    fclose(file) ;
    return ;
  }

  /* Setup file io */
  png_init_io(png_ptr, file);

  /* */
  png_set_IHDR(png_ptr, info_ptr, width, height, 8,
               PNG_COLOR_TYPE_RGB,
               PNG_INTERLACE_NONE,
               PNG_COMPRESSION_TYPE_DEFAULT,
               PNG_FILTER_TYPE_DEFAULT);

  /* Write everything */
  png_write_info(png_ptr, info_ptr);
  png_set_swap(png_ptr) ;
  for (size_t i = 0; i < height; i++) {
    png_write_row(png_ptr, (png_bytep)(pixels + i * width * 3));
  }
  png_write_end(png_ptr, info_ptr);

  /* Done */
  png_destroy_write_struct(&png_ptr, &info_ptr);
  fclose(file) ;
  return ;
}


// -------------------------------------------------------------------


void rdmInit()
{
  rdmSetBaseName("./rdm-recording") ;
}

void rdmFinish()
{
  rdmStopRecording() ;
}

void rdmSetFlag(rdmRecordingModeMask flags)
{
  rdmRecordingMode |= flags ;
}

void rdmSetBaseName(const char * name)
{
  strncpy(rdmBaseName, name, sizeof(rdmBaseName)) ;
}

/* ---------------------------------------------------------------- */
/*                              rdmStartRecording, rdmStopRecording */
/* ---------------------------------------------------------------- */
#pragma mark -

static void rdmMakeDir(const char * dirName)
{
  struct stat st = {0};
  if (stat(dirName, &st) == -1) {
    mkdir(dirName, 0700) ;
  }
}

void rdmStartRecording(size_t width, size_t height)
{
  char str [1024] ;
  rdmFrameWidth = width ;
  rdmFrameHeight = height ;
  rdmLastRecordedTic = 0 ;

  if (rdmRecordingMode) {
    rdmMakeDir(rdmBaseName) ;
  }

  if (rdmRecordingMode & kRecordingModeMaskLog) {
    snprintf(str, sizeof(str), "%s/log.txt", rdmBaseName) ;
    rdmLogFile = fopen(str, "w") ;
  }

  if (rdmRecordingMode & kRecordingModeMaskRGB) {
    snprintf(str, sizeof(str), "%s/rgb", rdmBaseName) ;
    rdmMakeDir(str) ;
  }

  if (rdmRecordingMode & kRecordingModeMaskDepth) {
    snprintf(str, sizeof(str), "%s/depth", rdmBaseName) ;
    rdmMakeDir(str) ;
    rdmDepthMapBuffer = calloc(width*height,sizeof(*rdmDepthMapBuffer)) ;
  }

  if (rdmRecordingMode & kRecordingModeMaskObjects) {
    snprintf(str, sizeof(str), "%s/objects", rdmBaseName) ;
    rdmObjectMapBuffer = calloc(width*height*3,sizeof(*rdmObjectMapBuffer)) ;
    rdmMakeDir(str) ;
  }

  rdmIsRecording = true ;
}

void rdmStopRecording()
{
  rdmIsRecording = false ;
  if (rdmObjectMapBuffer) {
    free(rdmObjectMapBuffer) ;
    rdmObjectMapBuffer = 0 ;
  }
  if (rdmDepthMapBuffer) {
    free(rdmDepthMapBuffer) ;
    rdmDepthMapBuffer = 0 ;
  }
  if (rdmLogFile) {
    fclose(rdmLogFile) ;
    rdmLogFile = NULL ;
  }
}

/* ---------------------------------------------------------------- */
/*                                                       rdmRecord* */
/* ---------------------------------------------------------------- */

void rdmRecordLog(size_t tic, char const * format, ...)
{
  if (rdmLogFile) {
    fprintf(rdmLogFile, "%06zu ", tic) ;
    va_list va ;
    va_start(va, format);
    vfprintf(rdmLogFile, format, va) ;
    va_end(va);
    fputc('\n',rdmLogFile) ;
  }
}

void rdmFlushLog()
{
  if (rdmLogFile) {
    fflush(rdmLogFile) ;
  }
}

void rdmRecordRGB(size_t tic, uint8_t const * pixels,
                       uint8_t const * palette)
{
  if (rdmIsRecording && (rdmRecordingMode & kRecordingModeMaskRGB)) {
    char str [1024] ;
    snprintf(str, sizeof(str), "%s/rgb/%06zu.png", rdmBaseName, tic) ;
    savePNG(str, pixels, rdmFrameWidth, rdmFrameHeight, palette) ;
    rdmLastRecordedTic = tic ;
  }
}

void rdmRecordDepth(size_t tic)
{
  if (rdmIsRecording && (rdmRecordingMode & kRecordingModeMaskDepth)) {
    char str [1024] ;
    snprintf(str, sizeof(str), "%s/depth/%06zu.png", rdmBaseName, tic) ;
    savePNG16(str, rdmDepthMapBuffer, rdmFrameWidth, rdmFrameHeight) ;
    rdmLastRecordedTic = tic ;
  }
}

void rdmRecordObjects(size_t tic)
{
  if (rdmIsRecording && (rdmRecordingMode & kRecordingModeMaskObjects)) {
    char str [1024] ;
    snprintf(str, sizeof(str), "%s/objects/%06zu.png", rdmBaseName, tic) ;
    savePNG24(str, rdmObjectMapBuffer, rdmFrameWidth, rdmFrameHeight) ;
    rdmLastRecordedTic = tic ;
  }
}



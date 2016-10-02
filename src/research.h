//
//  research.h
//  doom
//
//  Created by Andrea Vedaldi on 25/05/2016.
//  Copyright © 2016 Andrea Vedaldi. All rights reserved.
//

#ifndef research_h
#define research_h

#include "stdint.h"
#include "stdbool.h"

typedef enum {
  kRecordingModeMaskLog = 0x1,
  kRecordingModeMaskRGB = 0x2,
  kRecordingModeMaskDepth = 0x4,
  kRecordingModeMaskObjects = 0x8,
} rdmRecordingModeMask ;

typedef enum
{
  kObjectMask = (1 << 23) - 1,
  kObjectIdSky = 0 + (1 << 23),
  kObjectIdHorizontal = 1 + (1 << 23),
  kObjectIdVertical = 2 + (1 << 23)
} rdmObjectId ;

extern char const* researchObjectTypeNames [] ;
extern bool rdmIsRecording ;
extern bool rdmHidePlayer ;
extern bool rdmSyncFrames ;
extern rdmRecordingModeMask rdmRecordingMode ;
extern uint16_t * rdmDepthMapBuffer ;
extern uint8_t * rdmObjectMapBuffer ;

void rdmInit() ;
void rdmFinish() ;

void rdmSetBaseName(const char * name) ;
void rdmSetFlag(rdmRecordingModeMask flag) ;

void rdmStartRecording(size_t width, size_t height) ;
void rdmStopRecording() ;

void rdmRecordLog(size_t tic, const char *fmt, ...) ;
void rdmFlushLog() ;
void rdmRecordRGB(size_t tic, uint8_t const * pixels, uint8_t const * palette) ;
void rdmRecordDepth(size_t tic) ;
void rdmRecordObjects(size_t tic) ;

#endif /* research_h */

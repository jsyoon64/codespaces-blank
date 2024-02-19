/**
  ******************************************************************************
  * @file     : em2.h
  * @author   : jsyoon
  * @date     : 2024/02/15
  * @brief    : Header for template.c file.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2024 .
  * All rights reserved.
  *
  ******************************************************************************
  * @modification tracking
  *    author      date      number      description of change
  *   ---------  --------   ---------    ------------------------------------
  *    jsyoon     24/02/15   1.0.0       initial release
  *
  ******************************************************************************
  */
#ifndef _EVENT_MANAGER2_H_
#define _EVENT_MANAGER2_H_
/* Includes ------------------------------------------------------------------*/
/* standard includes */
#include <stdint.h>

/* scheduler includes */
/* driver includes */
/* application includes */

/* Private defines -----------------------------------------------------------*/
#define PC_SIMULATION

#define DEFAULT_HANDLER_NO_MEM_FREE             (1)

/* 1: handler function에서 memory free 수행 해야 함
  -1: event manager에서 memory free 수행 함.
*/
#define HANDLER_REQUIRED_MEMORYFREE             (-1)

/* 1: 각 group별 event enum이 0 부터 시작 해서 순서 대로 되어 있어 event갯수로 한꺼번에 register 됨
  -1: 각 group별 event enum이 연속적이이 않아서 event별로 register해야 함.
*/
#define FEATURE_SEQUENCE_EVENT_ENUM             (1)

/* Exported constants --------------------------------------------------------*/
#define MAX_ROOT_EVENT_GROUP_COUNT              20

/* Exported macro ------------------------------------------------------------*/
#ifdef PC_SIMULATION
#define EM_IS_MEMFREEREQUIRED(ev)                \
    if ((ev) && (ev->msg != NULL) && (ev->isconst == 0)) \
    {                                            \
        free(ev->msg);                           \
        ev->msg = NULL;                          \
        printf("buffer freed\n");                \
    }
#else
#define EM_IS_MEMFREEREQUIRED(ev)                \
    if ((ev) && (ev->msg != NULL) && (ev->isconst == 0)) \
    {                                            \
        vPortFree(ev->msg);                      \
        ev->msg = NULL;                          \
    }
#endif 

/* Exported types ------------------------------------------------------------*/
typedef struct
{
    uint16_t    isconst;
    uint16_t    len;
    void *      msg;
} em_event_arg_type;

typedef struct 
{
    const char  *name;
    int32_t     gid; 
} em_group_name_type;

typedef void (*evt_handler_fp)(const char*, int16_t, em_event_arg_type *);

typedef struct sEM_HANDLER_T
{
    evt_handler_fp          handler;
    struct sEM_HANDLER_T    *pNext;
} em_handler_list_type;

typedef struct sEM_ID_HANDLER_T
{
    int16_t                 event;
    uint16_t                event_id;
    em_handler_list_type    *handler;
    #ifndef FEATURE_NONSEQ_ENUM 
    struct sEM_ID_HANDLER_T *pNext;
    #endif
} em_event_id_type;

typedef struct 
{
    em_group_name_type      event_group;
    em_handler_list_type    *grphandler; // Group handler
    em_event_id_type        *evthandler;
    uint16_t                group_evt_cnt;
} em_event_group_type;

typedef struct 
{
    em_event_group_type group[MAX_ROOT_EVENT_GROUP_COUNT];      
    uint16_t            group_cnt;
} em_event_group_list_type;


// const char * : is a pointer to a const char
// char * const : is a constant pointer to a char.
// const char * const : is a constant pointer to a constant char (so nothing about it can be changed).

/* 
    int       *      mutable_pointer_to_mutable_int;
    int const *      mutable_pointer_to_constant_int;
    int       *const constant_pointer_to_mutable_int;
    int const *const constant_pointer_to_constant_int;
*/
// #define EM_DECLARE_EVENT_GROUP(id) extern const char* const id
// #define EM_DEFINE_EVENT_GROUP(id) const char* const id = #id
// #define EM_DECLARE_EVENT_GROUP(id) extern const char* id
// #define EM_DEFINE_EVENT_GROUP(id) const char* id = #id

/* Exported functions prototypes ---------------------------------------------*/

/* Event request */
void em_on_event(em_group_name_type *eventgroup, int16_t signal, evt_handler_fp handler);

/*---------------------------------------------*/
/* Event register */
void em_events_register(em_group_name_type *, int16_t );

/*---------------------------------------------*/
/* Event trigger */
void em_event_trigger(em_group_name_type *eventgroup, int16_t signal, em_event_arg_type *event);

/*---------------------------------------------*/
/* Event manager initialize */
void em_initialize(void);

#endif  /* _EVENT_MANAGER2_H_*/

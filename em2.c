
/**
  ******************************************************************************
  * @file       : em2.c
  * @author     : jsyoon
  * @date       : 2024/02/15
  * @brief      : event manager 2
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
  */

/* Includes ------------------------------------------------------------------*/
/* Standard includes. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "em2.h"

/* scheduler includes */
#ifndef PC_SIMULATION
#include "FreeRTOS.h"
#include "task.h"
#endif

/* driver includes */
/* Application include files. */
#ifndef PC_SIMULATION
#include "debugprint.h"
#endif
/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
static em_event_group_list_type root_event_list;
static em_event_arg_type trigger_event;

uint8_t *event_msg_backup;
em_event_arg_type *current_event;
em_event_group_type *group;
em_handler_list_type *gListHandler;
em_event_id_type *evt_handler;
em_handler_list_type *eListHandler;

/* Private function prototypes -----------------------------------------------*/
/* Private function code -----------------------------------------------------*/
/**
  * @brief  em_default_handler
  * @note
  * @param  None
  * @retval None
  */
void em_default_handler(const char *groupname, int16_t signal, em_event_arg_type *ev)
{
    #ifdef PC_SIMULATION
    printf("Default Handler: Event group(%s) event(0x%04x) arg(%p) triggered!\n", groupname, signal, ev);
    #else
    DEBUGHI(GEN,"Default Handler: Event group(%s) event(0x%04x) arg(%p) triggered!\n", groupname, signal, ev);
    #endif

    #if (DEFAULT_HANDLER_NO_MEM_FREE < 0)
    EM_IS_MEMFREEREQUIRED(ev);
    #endif
}

/**
  * @brief  em_NewEventMem
  * @note
  * @param  None
  * @retval None
  */
uint8_t *em_NewEventMem(em_event_arg_type *event)
{
    uint8_t *ev_buf = NULL;

    if ((event == NULL) || (event->msg == NULL) || (event->isconst == 1)) {
        return NULL;
    }

    #ifdef PC_SIMULATION
    ev_buf = malloc(event->len + 1);
    #else
    ev_buf = pvPortMalloc(event->len + 1);
    #endif 
    if (ev_buf == NULL)  {
        //
        //memory allocation error
        #ifdef PC_SIMULATION
        printf("Memory allocation error\n");
        #else
        DEBUGERR(GEN, AllocErrMsg("em_NewEventMem"));
        #endif  
    }
    else {
        memset(ev_buf, 0x00, event->len + 1);
        memcpy(ev_buf, event->msg, event->len);
        return ev_buf;        
    }
    /* TODO error return required */
    return ev_buf;
}


/**
  * @brief  createNode
  * @note
  * @param  None
  * @retval None
  */
em_handler_list_type *createNode(evt_handler_fp handler)
{
    #ifdef PC_SIMULATION
    em_handler_list_type *newNode = (em_handler_list_type *)malloc(sizeof(em_handler_list_type));
    #else
    em_handler_list_type *newNode = (em_handler_list_type *)pvPortMalloc(sizeof(em_handler_list_type));
    #endif 

    newNode->handler = handler;
    newNode->pNext = NULL; // 생성할 때는 next를 NULL로 초기화

    return newNode;   
}

/**
  * @brief  get_registered_group
  * @note   이미 등록 되어 있는 EVENT GROUP 인지 판단
  * @param  None
  * @retval None
  */
em_event_group_type *get_registered_group(em_group_name_type *eventgroup)
{
    if( (eventgroup->gid >= 0 ) && ( eventgroup->gid < MAX_ROOT_EVENT_GROUP_COUNT ) )
    {
        if( strcmp(root_event_list.group[eventgroup->gid].event_group.name, eventgroup->name) == 0) {
            return &root_event_list.group[eventgroup->gid];
        }        
        else {
            #ifdef PC_SIMULATION
            printf("Group name(%s) is not registered!!!\n", eventgroup->name);
            #else
            DEBUGERR(GEN,"Group name(%s) is not registered!!!\n", eventgroup->name);
            #endif              
            return NULL;
        }
    }
    return NULL;
}

/**
  * @brief  get_registered_group
  * @note   이미 등록 되어 있는 EVENT GROUP index
  * @param  None
  * @retval None
  */
int get_registered_groupID(em_group_name_type *eventgroup)
{
    if((eventgroup->gid >= 0 ) && (eventgroup->gid < MAX_ROOT_EVENT_GROUP_COUNT ) )
    {
        return eventgroup->gid;
    }
    return -1;
}

/**
  * @brief  getHandlerCount
  * @note   
  * @param  None
  * @retval None
  */
int getHandlerCount(em_handler_list_type *handler)
{
    int count = 0;
    em_handler_list_type *han = handler;
    while (han != NULL) {
        if(han->handler) {
            count++;
        }
        han = han->pNext;
    }
    return count;
}

/**
  * @brief  getEventHandler
  * @note   
  * @param  None
  * @retval None
  */
em_event_id_type *getEventHandler(em_event_group_type *group, int16_t event)
{
    em_event_group_type *ghan = group;
    em_event_id_type *idhand = group->evthandler;

    #if (FEATURE_SEQUENCE_EVENT_ENUM > 0)
    return &idhand[event];
    #else
    for(int i=0;i<ghan->group_evt_cnt;i++) {
        if(idhand->event == event) {
            return idhand;
        }
        else {
            idhand = idhand->pNext;
        }
    }
    #endif
    return NULL;
}

/**
  * @brief  is_event_backup_require
  * @note   하나의 signal에 여러개의 handler가 등록 되어 있는경우
  * @param  None
  * @retval None
  */
int is_event_backup_require(int16_t group_index, em_event_arg_type *event, int16_t signal)
{

    #if (HANDLER_REQUIRED_MEMORYFREE > 0)
    int count;

    if ((event == NULL) || (event->msg == NULL) || (event->isconst == 1)) {
        return -1;
    }
    em_event_group_type *group = &root_event_list.group[group_index];
    em_handler_list_type *handler = group->grphandler;
    
    /* default handler */
    #if (DEFAULT_HANDLER_NO_MEM_FREE > 0)
    if(handler) {
        handler = handler->pNext;
    }
    #endif
    count = getHandlerCount(handler);
    
    em_event_id_type *evt_handler = getEventHandler(group, signal);
    if(evt_handler) {
        count += getHandlerCount(evt_handler->handler);
    }

    if(count > 0) {
        return count;
    }
    #endif
    return -1;
}


/**
  * @brief  addToTailHandlerList
  * @note   
  * @param  None
  * @retval None
  */
void addToTailHandlerList(em_handler_list_type **head, em_handler_list_type *node)
{
    em_handler_list_type *temp = *head;
    if(temp) {
        do {
            if(temp->pNext == NULL) {
                temp->pNext = node;
                temp = temp->pNext;
            }
            temp = temp->pNext;
        } while (temp != NULL);
    }
    else {
        *head = node;
    }
}

#if (FEATURE_SEQUENCE_EVENT_ENUM <= 0)

/**
  * @brief  addToTailHandlerList
  * @note   
  * @param  None
  * @retval None
  */
void addToTailEventList(em_event_id_type **head, em_event_id_type *node)
{
    em_event_id_type *temp = *head;
    if(temp) {
        do {
            if(temp->pNext == NULL) {
                temp->pNext = node;
                temp = temp->pNext;
            }
            temp = temp->pNext;
        } while (temp != NULL);
    }
    else {
        *head = node;
    }
}
#endif

/* Global function code ------------------------------------------------------*/

/**
  * @brief  em_on_event
  * @note   Event request
  * @param  None
  * @retval None
  */
void em_on_event(em_group_name_type *eventgroup, int16_t signal, evt_handler_fp handler)
{
    em_handler_list_type *new_node;
    em_event_group_type *group;
    em_event_id_type *evt_handler;

    if( eventgroup->name && handler ) {
        /* 등록된 그룹인지 확인 한다. */
        em_event_group_type *group = get_registered_group(eventgroup);
        if( group != NULL ) {
            new_node = createNode(handler);

            /* GROUP의 모든 EVENT에 대해 통보*/
            if(signal < 0) {
                addToTailHandlerList(&group->grphandler, new_node);
            }
            /* single event에 대한 통보 */
            else {
                evt_handler = getEventHandler(group, signal);
                if(evt_handler) {
                    evt_handler->event = signal;
                    addToTailHandlerList(&evt_handler->handler, new_node);
                }
            }            
            #ifdef PC_SIMULATION
            printf("Event group(%s) Event(0x%04x) is requested!!!\n", eventgroup->name, signal);
            #else
            DEBUGMED(GEN,"Event group(%s) Event(0x%04x) is requested!!!\n", eventgroup->name, signal);
            #endif         
        }
        else {
            /* group 등록이 되어 있지 않음 */
            #ifdef PC_SIMULATION
            printf("Event group(%s) not registered!!!\n", eventgroup->name);
            #else
            DEBUGMED(GEN,"Event group(%s)not registered!!!\n", eventgroup->name);
            #endif             
        }
    }
    else {
        #ifdef PC_SIMULATION
        printf("Group, Handler must be defined!!!\n");
        #else
        DEBUGMED(GEN,"Group, Handler must be defined!!!\n");
        #endif   
    }
}

/**
  * @brief  em_events_register
  * @note   Event register
  * @param  None
  * @retval None
  */
#if (FEATURE_SEQUENCE_EVENT_ENUM > 0)
void em_events_register(em_group_name_type *eventgroup, int16_t event_count)
#else
void em_events_register(em_group_name_type *eventgroup, int16_t enum_event)
#endif
{
    uint16_t grp_cnt = root_event_list.group_cnt;

    if( eventgroup->name ) {
        /*새로운 GROUP인지, 이미 등록된 그룹인지 */
        em_event_group_type *group = get_registered_group(eventgroup);
        if( group == NULL ) {
            group = &root_event_list.group[grp_cnt];

            group->event_group.name = eventgroup->name;
            group->event_group.gid = grp_cnt;

            eventgroup->gid = grp_cnt;

            /* Add Group Handler */
            em_handler_list_type *gHandler = createNode(em_default_handler);
            group->grphandler = gHandler;

            /* Add event andler */
            #if (FEATURE_SEQUENCE_EVENT_ENUM > 0)
                #ifdef PC_SIMULATION
                em_event_id_type *evhandle = (em_event_id_type *)malloc(sizeof(em_event_id_type)*event_count);
                #else
                em_event_id_type *evhandle = (em_event_id_type *)pvPortMalloc(sizeof(em_event_id_type)*event_count);
                #endif 
                memset(evhandle,0,sizeof(em_event_id_type)*event_count);
                group->evthandler = evhandle; /* array로 access 하면 됨 */
                group->group_evt_cnt = event_count;

            #else
                #ifdef PC_SIMULATION
                em_event_id_type *evt_handler = (em_event_id_type *)malloc(sizeof(em_event_id_type));
                #else
                em_event_id_type *evt_handler = (em_event_id_type *)pvPortMalloc(sizeof(em_event_id_type));
                #endif 
                evt_handler->event = enum_event;
                evt_handler->handler = NULL;
                group->evthandler = evt_handler;
                group->group_evt_cnt = 1;
            #endif
            root_event_list.group_cnt++;
        }
        else {
            #if (FEATURE_SEQUENCE_EVENT_ENUM < 0)
                /* GROUP이 이미 등록 되어 있음 */

                /* Add event andler */
                #ifdef PC_SIMULATION
                em_event_id_type *evt_handler = (em_event_id_type *)malloc(sizeof(em_event_id_type));
                #else
                em_event_id_type *evt_handler = (em_event_id_type *)pvPortMalloc(sizeof(em_event_id_type));
                #endif 
                evt_handler->event = enum_event;
                evt_handler->handler = NULL;

                addToTailEventList(&group->evthandler, evt_handler);

                group->group_evt_cnt++;
            #else
                #ifdef PC_SIMULATION
                printf("%s Group already registered!!!\n", eventgroup->name);
                #else
                DEBUGERR(GEN,"%s Group already registered!!!\n", eventgroup->name);
                #endif                  
            #endif
        }
    }
    else {
        /* group은 null이면 안됨 */
    }
}

/**
  * @brief  em_event_trigger
  * @note   Event trigger
  * @param  None
  * @retval None
  */
void em_event_trigger(em_group_name_type *eventgroup, int16_t signal, em_event_arg_type *event)
{
    /* move from stack to static */
    // uint8_t *event_msg_backup = NULL;
    // em_event_arg_type *current_event = NULL;
    // em_event_group_type *group = NULL;
    // em_handler_list_type *gListHandler = NULL;
    // em_event_id_type *evt_handler = NULL;
    // em_handler_list_type *eListHandler = NULL;
    int16_t isbackupreq = -1;

    /* Search registered groupname */
    int16_t group_index = get_registered_groupID(eventgroup);

    /* move from stack to static */
    event_msg_backup = NULL;
    current_event = NULL;
    group = NULL;
    gListHandler = NULL;
    evt_handler = NULL;
    eListHandler = NULL;

    if(group_index < 0) {
        #ifdef PC_SIMULATION
        printf("Group name(%s) is not registered!!!\n", eventgroup->name);
        #else
        DEBUGERR(GEN,"Group name(%s) is not registered!!!\n", eventgroup->name);
        #endif  
        return;
    }

    if(event != NULL) {
        memcpy(&trigger_event, event, sizeof(em_event_arg_type));
        current_event = &trigger_event;

        #if (HANDLER_REQUIRED_MEMORYFREE > 0)
        isbackupreq = is_event_backup_require(group_index, event, signal);
        #else
        /* memory free 는 event manager에서 수행 됨 */
        current_event->isconst = 1;
        #endif
    }

    group = &root_event_list.group[group_index];
    /* 1. Group handler 
    */    
    gListHandler = group->grphandler;

    if(gListHandler != NULL) {
        /* default handler */
        #if (DEFAULT_HANDLER_NO_MEM_FREE > 0)
        gListHandler->handler(eventgroup->name, signal, current_event);
        gListHandler = gListHandler->pNext;
        #endif

        while (gListHandler != NULL) {
            if(isbackupreq > 0) {
                event_msg_backup = em_NewEventMem(current_event);
            }
            gListHandler->handler(eventgroup->name, signal, current_event);
            gListHandler = gListHandler->pNext;

            if(event_msg_backup != NULL) {
                current_event->msg = event_msg_backup;
            }
        }
    }

    /* isbackupreq > 0 일 경우  1개의 event_msg_backup 남아 있음 
       current_event->msg에 내용이 있음.
    */

    /* 2. Event handler 
    */    
    evt_handler = getEventHandler(group, signal);
    eListHandler = evt_handler->handler;
    while (eListHandler != NULL) {
        if(isbackupreq > 0) {
            event_msg_backup = em_NewEventMem(current_event);
        }
        eListHandler->handler(eventgroup->name, signal, current_event);
        eListHandler = eListHandler->pNext;

        if(event_msg_backup != NULL) {
            current_event->msg = event_msg_backup;
        }
    }
    /* isbackupreq > 0 일 경우  1개의 event_msg_backup 남아 있음 */
    if(event_msg_backup) {
        #ifdef PC_SIMULATION
        free(event_msg_backup);
        #else
        vPortFree(event_msg_backup);
        #endif 
    }

    #if (HANDLER_REQUIRED_MEMORYFREE <= 0)
    EM_IS_MEMFREEREQUIRED(event);
    #endif
}

/**
  * @brief  em_initialize
  * @note   Event manager initialize
  * @param  None
  * @retval None
  */
void em_initialize(void)
{
    memset(&root_event_list, 0x00, sizeof(em_event_group_list_type));

    for(int i=0; i<MAX_ROOT_EVENT_GROUP_COUNT; i++ ) {
        root_event_list.group[i].event_group.gid = -1;
    }

}


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
    printf("default: %s event(0x%04x) triggered!\n", groupname, signal);
    #else
    DEBUGHI(GEN,"default: %s event(0x%04x) triggered!\n", groupname, signal);
    #endif 
    EM_IS_MEMFREEREQUIRED(ev);
}

#if (HANDLER_REQUIRED_MEMORYFREE > 0)
/**
  * @brief  em_NewMem
  * @note
  * @param  None
  * @retval None
  */
uint8_t *em_NewMem(em_event_arg_type *event)
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
        DEBUGERR(GEN, AllocErrMsg("em_NewMem"));
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
#endif


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
  * @brief  check_is_registered_group
  * @note   이미 등록 되어 있는 EVENT GROUP 인지 판단
  * @param  None
  * @retval None
  */
int check_is_registered_group(const char *groupname)
{
    for(int i=0;i<root_event_list.group_cnt;i++) {
        if( strcmp(root_event_list.group[i].name, groupname) == 0) {
            return i;
        }
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
    count = getHandlerCount(group->grphandler);
    
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
void em_on_event(const char *groupname, int16_t signal, evt_handler_fp handler)
{
    em_handler_list_type *new_node;
    em_event_group_type *group;
    em_event_id_type *evt_handler;

    if( groupname && handler ) {
        /* 등록된 그룹인지 확인 한다. */
        int16_t group_index = check_is_registered_group(groupname);

        if( group_index >= 0 ) {
            new_node = createNode(handler);
            group = &root_event_list.group[group_index];

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
        }
        else {
            /* group 등록이 되어 있지 않음 */
            #ifdef PC_SIMULATION
            printf("%s Group not registered!!!\n", groupname);
            #else
            DEBUGMED(GEN,"%s Group not registered!!!\n", groupname);
            #endif             
        }

        #ifdef PC_SIMULATION
        printf("%s Group Event(%d) is requested!!!\n", groupname, signal);
        #else
        DEBUGMED(GEN,"%s Group Event(%d) is requested!!!\n", groupname, signal);
        #endif         
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
void em_events_register(const char *groupname, int16_t event_count)
#else
void em_events_register(const char *groupname, int16_t enum_event)
#endif
{
    uint16_t grp_cnt = root_event_list.group_cnt;

    if( groupname ) {
        /*새로운 GROUP인지, 이미 등록된 그룹인지 */
        int16_t group_index = check_is_registered_group(groupname);
        if( group_index < 0 ) {
            root_event_list.group[grp_cnt].name = groupname;
            /* Add Group Handler */
            em_handler_list_type *gHandler = createNode(em_default_handler);
            root_event_list.group[grp_cnt].grphandler = gHandler;

            /* Add event andler */
            #if (FEATURE_SEQUENCE_EVENT_ENUM > 0)
                #ifdef PC_SIMULATION
                em_event_id_type *evhandle = (em_event_id_type *)malloc(sizeof(em_event_id_type)*event_count);
                #else
                em_event_id_type *evhandle = (em_event_id_type *)pvPortMalloc(sizeof(em_event_id_type)*event_count);
                #endif 
                memset(evhandle,0,sizeof(em_event_id_type)*event_count);
                root_event_list.group[grp_cnt].evthandler = evhandle; /* array로 access 하면 됨 */
                root_event_list.group[grp_cnt].group_evt_cnt = event_count;

            #else
                #ifdef PC_SIMULATION
                em_event_id_type *evt_handler = (em_event_id_type *)malloc(sizeof(em_event_id_type));
                #else
                em_event_id_type *evt_handler = (em_event_id_type *)pvPortMalloc(sizeof(em_event_id_type));
                #endif 
                evt_handler->event = enum_event;
                evt_handler->handler = NULL;
                root_event_list.group[grp_cnt].evthandler = evt_handler;

                root_event_list.group[grp_cnt].group_evt_cnt = 1;
            #endif
            root_event_list.group_cnt++;
        }
        else {
            #if (FEATURE_SEQUENCE_EVENT_ENUM < 0)
                /* GROUP이 이미 등록 되어 있음 */
                em_event_group_type *group = &root_event_list.group[group_index];

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
                printf("%s Group already registered!!!\n", groupname);
                #else
                DEBUGERR(GEN,"%s Group already registered!!!\n", groupname);
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
void em_event_trigger(const char *groupname, int16_t signal, em_event_arg_type *event)
{
    uint8_t *event_msg_backup = NULL;
    em_event_arg_type current_event;
    int16_t isbackupreq = -1;

    memcpy(&current_event, event, sizeof(em_event_arg_type));

    #if (HANDLER_REQUIRED_MEMORYFREE < 0)
    /* memory free 는 event manager에서 수행 됨 */
    current_event.isconst = 1;
    #endif

    /* Search registered groupname */
    int16_t group_index = check_is_registered_group(groupname);
    if(group_index < 0) {
        #ifdef PC_SIMULATION
        printf("Group name(%s) is not registered!!!\n", groupname);
        #else
        DEBUGERR(GEN,"Group name(%s) is not registered!!!\n", groupname);
        #endif  
        return;
    }

    #if (HANDLER_REQUIRED_MEMORYFREE < 0)
    isbackupreq = is_event_backup_require(group_index, event, signal);
    #endif

    em_event_group_type *group = &root_event_list.group[group_index];
    /* 1. Group handler 
    */    
    em_handler_list_type *gListHandler = group->grphandler;
    while (gListHandler != NULL) {
        if(isbackupreq > 0) {
            event_msg_backup = em_NewMem(event);
        }
        gListHandler->handler(groupname, signal, &current_event);
        gListHandler = gListHandler->pNext;

        if(event_msg_backup != NULL) {
            current_event.msg = event_msg_backup;
        }
    }

    /* isbackupreq > 0 일 경우  1개의 event_msg_backup 남아 있음 */
    #ifdef PC_SIMULATION
        free(event_msg_backup);
    #else
        vPortFree(event_msg_backup);
    #endif 

    /* 2. Event handler 
    */    
    em_event_id_type *evt_handler = getEventHandler(group, signal);
    em_handler_list_type *eListHandler = evt_handler->handler;
    while (eListHandler != NULL) {
        if(isbackupreq > 0) {
            event_msg_backup = em_NewMem(event);
        }
        eListHandler->handler(groupname, signal, event);
        eListHandler = eListHandler->pNext;

        if(event_msg_backup != NULL) {
            current_event.msg = event_msg_backup;
        }
    }
    /* isbackupreq > 0 일 경우  1개의 event_msg_backup 남아 있음 */
    #ifdef PC_SIMULATION
        free(event_msg_backup);
    #else
        vPortFree(event_msg_backup);
    #endif 

    // free backup event
    EM_IS_MEMFREEREQUIRED(event);
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
}

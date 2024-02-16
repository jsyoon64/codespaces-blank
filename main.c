
#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "em2.h"
#ifdef PC_SIMULATION
#include "em2.c"
#endif

/*---------------------------------------------*/
void group_handler(const char *groupname, int16_t signal, em_event_arg_type *msg)
{
    char* msg2 = ((msg)&&(msg->msg))?(char*)msg->msg:"" ;
    #ifdef PC_SIMULATION
    printf("group_handler: %s signal(0x%04x) msg(\"%s\") triggered!\n", groupname, signal, msg2);
    #endif
    EM_IS_MEMFREEREQUIRED(msg);
}
void test2_handler(const char *groupname, int16_t signal, em_event_arg_type *msg)
{
    char* msg2 = ((msg)&&(msg->msg))?(char*)msg->msg:"" ;
    #ifdef PC_SIMULATION
    printf("test2_handler: %s signal(0x%04x) msg(\"%s\") triggered!\n", groupname, signal, msg2 );
    #endif
    EM_IS_MEMFREEREQUIRED(msg);
}
void test3_handler(const char *groupname, int16_t signal, em_event_arg_type *msg)
{
    char* msg2 = ((msg)&&(msg->msg))?(char*)msg->msg:"" ;
    #ifdef PC_SIMULATION
    printf("test3_handler: %s signal(0x%04x) msg(\"%s\") triggered!\n", groupname, signal, msg2);
    #endif
    EM_IS_MEMFREEREQUIRED(msg);    
}


/*
    local signal :task 자체 -> 0x0000 ~0x7FFF
    global signal          ->  0x8000 ~0xFFFF
       handler에서 signal | 0x8000 해서 Q로 전송 하고
       statemachine에서는 signal | 0x8000 인지 확인
       test.c 참조
*/
void main()
{    
    em_initialize();

    /* global 로 선언하여 signal post 하는데에서 사용 해야 한다. */
    const char *ether_event_group = "ETHERNET_EVENTS";
    typedef enum {
        ETHERNET_EVENT_00,
        ETHERNET_EVENT_01,
        ETHERNET_EVENT_02,
        ETHERNET_EVENT_03,
        ETHERNET_EVENT_04,
        ETHERNET_EVENT_05,
        ETHERNET_EVENT_MAX
    }ethernet_event_type;

    const char *audio_event_group = "AUDIO_EVENTS";
    typedef enum {
        AUDIO_EVENT_00,
        AUDIO_EVENT_01,
        AUDIO_EVENT_02,
        AUDIO_EVENT_03,
        AUDIO_EVENT_04,
        AUDIO_EVENT_05,
        AUDIO_EVENT_MAX
    }audio_event_type;


    /* Event register */
    #if (FEATURE_SEQUENCE_EVENT_ENUM > 0)
    em_events_register(ether_event_group, (int16_t)ETHERNET_EVENT_MAX);
    #else
    em_events_register(ether_event_group, (int16_t)ETHERNET_EVENT_00);
    em_events_register(ether_event_group, (int16_t)ETHERNET_EVENT_01);
    em_events_register(ether_event_group, (int16_t)ETHERNET_EVENT_02);
    em_events_register(ether_event_group, (int16_t)ETHERNET_EVENT_03);
    em_events_register(ether_event_group, (int16_t)ETHERNET_EVENT_04);
    em_events_register(ether_event_group, (int16_t)ETHERNET_EVENT_05);
    #endif

    #if (FEATURE_SEQUENCE_EVENT_ENUM > 0)
    em_events_register(audio_event_group, (int16_t)AUDIO_EVENT_MAX);
    #else
    em_events_register(audio_event_group, (int16_t)AUDIO_EVENT_00);
    em_events_register(audio_event_group, (int16_t)AUDIO_EVENT_01);
    em_events_register(audio_event_group, (int16_t)AUDIO_EVENT_02);
    em_events_register(audio_event_group, (int16_t)AUDIO_EVENT_03);
    em_events_register(audio_event_group, (int16_t)AUDIO_EVENT_04);
    em_events_register(audio_event_group, (int16_t)AUDIO_EVENT_05);
    #endif


    /* 
        1. Ethernet events test
    */
    printf("\nEthernet Events test-------------------------------\n");

    /* Event 발생시 통보 요청 */
    em_on_event(ether_event_group, -1, group_handler);
    em_on_event(ether_event_group, ETHERNET_EVENT_01, test2_handler);
    em_on_event(ether_event_group, ETHERNET_EVENT_03, test2_handler);
    em_on_event(ether_event_group, ETHERNET_EVENT_03, test3_handler);

    printf("\nTrigger ETHERNET_EVENT_01 with argument NULL\n");
    em_event_trigger(ether_event_group, ETHERNET_EVENT_01, NULL);

    printf("\nTrigger ETHERNET_EVENT_03 with argument NULL\n");
    em_event_trigger(ether_event_group, ETHERNET_EVENT_03, NULL);

    em_event_arg_type arg1;

    /* allocated memory test */
    arg1.isconst = 0;
    arg1.len = 10;
    arg1.msg = malloc(arg1.len);
    memset(arg1.msg,0,arg1.len);
    memcpy(arg1.msg,"QQQQQ",arg1.len);

    printf("\nTrigger ETHERNET_EVENT_03 with event argument\n");
    em_event_trigger(ether_event_group, ETHERNET_EVENT_03, &arg1);


    /* const buffer test */
    arg1.isconst = 1;
    arg1.len = 10;
    arg1.msg = malloc(arg1.len);
    memset(arg1.msg,0,arg1.len);
    memcpy(arg1.msg,"CONST",arg1.len);

    printf("\nTrigger ETHERNET_EVENT_03 with const argument\n");
    em_event_trigger(ether_event_group, ETHERNET_EVENT_03, &arg1);
    free(arg1.msg);


    /* 
        2. Audio events test
    */
    printf("\nAudio Events test-------------------------------\n");

    /* Event 발생시 통보 요청 */
    em_on_event(audio_event_group, -1, group_handler);
    em_on_event(audio_event_group, AUDIO_EVENT_00, test2_handler);
    em_on_event(audio_event_group, AUDIO_EVENT_01, test2_handler);
    em_on_event(audio_event_group, AUDIO_EVENT_01, test3_handler);

    printf("\nTrigger AUDIO_EVENT_00 with argument NULL \n");
    em_event_trigger(audio_event_group, AUDIO_EVENT_00, NULL);

    printf("\nTrigger AUDIO_EVENT_01 with argument NULL\n");
    em_event_trigger(audio_event_group, AUDIO_EVENT_01, NULL);

    printf("\nTrigger AUDIO_EVENT_03. No action\n");
    em_event_trigger(audio_event_group, AUDIO_EVENT_03, NULL);

     /* allocated memory test */
    arg1.isconst = 0;
    arg1.len = 20;
    arg1.msg = malloc(arg1.len);
    memset(arg1.msg,0,arg1.len);
    memcpy(arg1.msg,"AUDIO EVENT TEST",arg1.len);

    printf("\nTrigger AUDIO_EVENT_00 with event allocated argument\n");
    em_event_trigger(audio_event_group, AUDIO_EVENT_00, &arg1);


    /* const buffer test */
    arg1.isconst = 1;
    arg1.len = 20;
    arg1.msg = malloc(arg1.len);
    memset(arg1.msg,0,arg1.len);
    memcpy(arg1.msg,"CONST AUDIO EVENT",arg1.len);

    printf("\nTrigger AUDIO_EVENT_01 with const event argument\n");
    em_event_trigger(audio_event_group, AUDIO_EVENT_01, &arg1);
    free(arg1.msg);



}

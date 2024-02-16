
#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "em2.h"
#ifdef PC_SIMULATION
#include "em2.c"
#endif

/*---------------------------------------------*/
void test1_handler(const char *groupname, int16_t signal, em_event_arg_type *msg)
{
    char* msg2 = ((msg)&&(msg->msg))?(char*)msg->msg:"" ;
    #ifdef PC_SIMULATION
    printf("test1_handler %s signal(0x%04x) msg(\"%s\") triggered!\n", groupname, signal, msg2);
    #endif
    EM_IS_MEMFREEREQUIRED(msg);
}
void test2_handler(const char *groupname, int16_t signal, em_event_arg_type *msg)
{
    char* msg2 = ((msg)&&(msg->msg))?(char*)msg->msg:"" ;
    #ifdef PC_SIMULATION
    printf("test2_handler %s signal(0x%04x) msg(\"%s\") triggered!\n", groupname, signal, msg2 );
    #endif
    EM_IS_MEMFREEREQUIRED(msg);
}
void test3_handler(const char *groupname, int16_t signal, em_event_arg_type *msg)
{
    char* msg2 = ((msg)&&(msg->msg))?(char*)msg->msg:"" ;
    #ifdef PC_SIMULATION
    printf("test3_handler %s signal(0x%04x) msg(\"%s\") triggered!\n", groupname, signal, msg2);
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
    const char *test_event_group = "ETHERNET_EVENTS";
    typedef enum {
        TEST_EVENT_00,
        TEST_EVENT_01,
        TEST_EVENT_02,
        TEST_EVENT_03,
        TEST_EVENT_04,
        TEST_EVENT_05,
        TEST_EVENT_MAX
    }test_event_type;

    /* Event register */
    #if (FEATURE_SEQUENCE_EVENT_ENUM > 0)

    em_events_register(test_event_group, (int16_t)TEST_EVENT_MAX);
    #else
    em_events_register(test_event_group, (int16_t)TEST_EVENT_00);
    em_events_register(test_event_group, (int16_t)TEST_EVENT_01);
    em_events_register(test_event_group, (int16_t)TEST_EVENT_02);
    em_events_register(test_event_group, (int16_t)TEST_EVENT_03);
    em_events_register(test_event_group, (int16_t)TEST_EVENT_04);
    em_events_register(test_event_group, (int16_t)TEST_EVENT_05);
    #endif

    /* Event 발생시 통보 요청 */
    em_on_event(test_event_group, -1, test1_handler);
    em_on_event(test_event_group, TEST_EVENT_01, test2_handler);
    em_on_event(test_event_group, TEST_EVENT_03, test2_handler);
    em_on_event(test_event_group, TEST_EVENT_03, test3_handler);

    printf("\nTrigger TEST_EVENT_01\n");
    em_event_trigger(test_event_group, TEST_EVENT_01, NULL);

    printf("\nTrigger TEST_EVENT_03\n");
    em_event_trigger(test_event_group, TEST_EVENT_03, NULL);

    em_event_arg_type arg1;
    arg1.isconst = 0;
    arg1.len = 10;
    arg1.msg = malloc(arg1.len);


    memset(arg1.msg,0,arg1.len);
    memcpy(arg1.msg,"QQQQQ",arg1.len);
    printf("\nbuffer alloced.\n");
    printf("\nTrigger TEST_EVENT_03 with event argument\n");
    em_event_trigger(test_event_group, TEST_EVENT_03, &arg1);

    arg1.isconst = 1;
    arg1.len = 10;
    arg1.msg = malloc(arg1.len);
    memset(arg1.msg,0,arg1.len);
    memcpy(arg1.msg,"CONST",arg1.len);

    printf("\nTrigger TEST_EVENT_03 with const argument\n");
    em_event_trigger(test_event_group, TEST_EVENT_03, &arg1);
}

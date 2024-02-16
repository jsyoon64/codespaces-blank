# EVENT MANAGER 2 
- Event Group이 존재 하고 그 그룹에는 그룹 전체의 handler와 다수의 event가 존재 한다.
- 각각의 event에는 다수의 handler가 존재 할 수 있다.

```text
-. 예로 "ETHERNET_EVT"그룹이 존재 할 경우
    1. 그룹 전체 handler: 그룹 모든 event에 대해 handler수행
```

```mermaid
---
title: Group Structure
---

flowchart LR
	subgraph group["MainGroup"]
	GHandlerList["Group Name lists"]
	GGroupCount[" Group Counts"]
	end

	subgraph subgroup["group name list array"]
	GHandlerList-->GName["Group Name"]
	GHandlers["Handler Lists"]
	GEventIDList["Event ID Lists"]
	GEventIDCount["Event ID Counts"]
	end

	subgraph IdList["Event ID Lists"]
	GEventIDList --> IDid["id"]
	IDHandlers["Handler Lists"]
	end

	subgraph Handler_Lists
	IDHandlers-->HandlerFP["Handler Function Ptr"]
	selfHandlerptr["Next handlerPtr"]
	end
	
	group-->subgroup
```



```mermaid
---
title: Group class diagram
---
classDiagram
    class em_event_group_list_type	{
	    em_event_group_type group[xx]
    	uint16_t            grp_cnt
	}

	class em_event_group_type {
		em_group_name_type      name
    	em_handler_list_type    *handler
    	em_event_id_type        *id
    	uint16_t                evt_cnt
	}

	class em_handler_list_type {
    	evt_handler_fp          handler
    	struct sEM_HANDLER_T    *pNext
	}

	class em_event_id_type {
	    int16_t                 id;
    	em_handler_list_type    *handler;
	}

	em_event_group_list_type --* em_event_group_type
	em_event_group_type --* em_event_id_type
```




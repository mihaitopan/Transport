#pragma once

#define CREATE_OBJECT_FNAME "CreateObject"
#define DESTROY_OBJECT_FNAME "DestroyObject"

#ifdef __cplusplus 
extern "C" {
#endif
	typedef int(*p_CreateObject)(const char* name, void** ptr);
	typedef int(*p_DestroyObject)(const char* name, void* ptr);
#ifdef __cplusplus
}
#endif
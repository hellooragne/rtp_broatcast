
#ifndef _LIB_HASH_H
#define _LIB_HASH_H

#ifdef  WIN32
    #define _CDECL  __cdecl
#else
    #define _CDECL
#endif


#ifdef __cplusplus
extern "C"{
#endif

#define nMAX_MGCP_HASH_SIZE   512


typedef struct _hash
{
    UINT32   hash_size;
    PS_LIST  all;
    PS_LIST  entry[2];
    
} HASH;


typedef struct _hash_node
{

    NODE     entry_list_node;
    NODE     all_list_node;

} HASH_NODE;


typedef struct _hash_string_node
{
    HASH_NODE  hash_node;
    char       string[128];
    UINT32     length;

} HASH_STRING_NODE;


typedef struct _hash_int_node
{
    HASH_NODE  hash_node;
    UINT32     key;

} HASH_INT_NODE;


// init

HASH*  _CDECL allocate_hash_table( UINT32  hash_size );

// item list in the hash table

UINT32 _CDECL hash_table_count( HASH *hash_table );
HASH_NODE* _CDECL hash_table_first(HASH *hash_table);
HASH_NODE* hash_table_last( HASH *hash_table);
HASH_NODE* hash_table_next( HASH_NODE *node );
HASH_NODE* hash_table_prev( HASH_NODE *node );

// string hash table

RET_CODE _CDECL hash_string_add(    HASH              *hash_table,
                             HASH_STRING_NODE  *node );
HASH_STRING_NODE* hash_string_delete( HASH              *hash_table,
							          char              *string,
									  UINT32             length );
HASH_STRING_NODE* hash_string_query(  HASH      *hash_table,
                                      char      *string,
                                      UINT32     length );

// interger hash table

RET_CODE hash_int_add(    HASH           *hash_table,
                          HASH_INT_NODE  *node );
HASH_INT_NODE* hash_int_delete( HASH      *hash_table,
								UINT32     key );
HASH_INT_NODE* hash_int_query(  HASH      *hash_table,
                                UINT32     key );

#ifdef __cplusplus
}
#endif

#endif // _LIB_HASH_H


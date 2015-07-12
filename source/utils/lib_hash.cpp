
#include "types.h"
#include "lib_hash.h"

#include <string.h> // lxt add for  _strncasecmp

static UINT32 get_string_hash_value( char *string, int length );



// hash utilities

HASH* allocate_hash_table(  UINT32  hash_size )
{
    HASH *table;
	UINT32  i;
    
    if ( hash_size > nMAX_MGCP_HASH_SIZE )
        return NULL;
    
    table = (HASH *) malloc( sizeof(HASH) + sizeof(PS_LIST) * hash_size );

    if ( table )
    {
        for ( i=0; i<hash_size; i++ )
        {
            lstInit( &table->entry[i] );
        }
		lstInit( &table->all );
		table->hash_size = hash_size;
    }
    return table;
    
}


UINT32     hash_table_count( HASH *hash_table )
{
    if ( hash_table )
        return (lstCount( &hash_table->all ));
    return  0;
}


HASH_NODE* hash_table_first( HASH *hash_table)
{
    HASH_NODE *node = NULL;
    
    if ( hash_table->all.pHead != NULL )
        node = (HASH_NODE*) BYOFFSET( hash_table->all.pHead, HASH_NODE, all_list_node );
    return node;
}

HASH_NODE* hash_table_last( HASH *hash_table)
{
    HASH_NODE *node = NULL;
    
    if ( hash_table->all.pTail != NULL )
        node = (HASH_NODE*) BYOFFSET( hash_table->all.pTail, HASH_NODE, all_list_node );
    return node;
}

HASH_NODE* hash_table_next( HASH_NODE *node )
{
    HASH_NODE *next = NULL;
    NODE*     pNode;

    pNode = lstNext( &node->all_list_node );
    if ( pNode != NULL )
        next = (HASH_NODE*) BYOFFSET( pNode, HASH_NODE, all_list_node );
    return ( next );
}

HASH_NODE* hash_table_prev( HASH_NODE *node )
{
    HASH_NODE *prev = NULL;
    NODE*    pNode;

    pNode = lstPrevious( &node->all_list_node );
    if ( pNode != NULL )
        prev =  (HASH_NODE*) BYOFFSET( pNode, HASH_NODE, all_list_node );
    return ( prev );
}


RET_CODE hash_string_add( HASH              *hash_table,
                          HASH_STRING_NODE  *node ) 
{
    
    int         rc = 0;
    UINT32      hash_value;
    PS_LIST     *hash_entry;
    HASH_STRING_NODE  *query_cb;

    if ( hash_table == NULL )
    {
        rc = -1;
        goto ErrExit;
    }

    query_cb = hash_string_query( hash_table,
                                  node->string,
                                  node->length );
    if ( query_cb )
    {
        // the transaction ID is already in there
        rc = -2;
        goto ErrExit;
    }
    
    hash_value = get_string_hash_value(  node->string,  node->length );
    hash_value = hash_value % hash_table->hash_size;
    hash_entry = &hash_table->entry[hash_value];
    lstAdd( hash_entry, &node->hash_node.entry_list_node );
    lstAdd( &hash_table->all, &node->hash_node.all_list_node );
    
 ErrExit:
    return rc;

}

HASH_STRING_NODE* hash_string_query( HASH              *hash_table,
                                     char              *string,
                                     UINT32             length )
{

    UINT32      hash_value;
    PS_LIST     *hash_entry;
    HASH_STRING_NODE   *hash_node;
	NODE       *node;
    
    if ( hash_table == NULL )
    {
        return NULL;
    }

    hash_value = get_string_hash_value(  string, length );
    hash_value = hash_value % hash_table->hash_size;
    hash_entry = &hash_table->entry[hash_value];

    if ( lstCount( hash_entry ) > 0 )
    {
        for ( node = lstFirst(hash_entry);
              node != NULL;
              node = lstNext( node) )
        {
			hash_node = (HASH_STRING_NODE *) BYOFFSET ( node, HASH_STRING_NODE, hash_node.entry_list_node );
            if ( hash_node->length == length &&
                 strncasecmp( hash_node->string, string, length ) == 0 )
            {
                return  hash_node;
            }
        }
    }
    return NULL;
        
}


HASH_STRING_NODE* hash_string_delete( HASH              *hash_table,
									  char              *string,
									  UINT32             length )
{
    

    UINT32             hash_value;
    PS_LIST           *hash_entry;
    HASH_STRING_NODE  *hash_node = NULL;
	NODE              *node;  
    int                rc = 1;  // in case not found any 
    
    //FUNC_ENT();
    
    if ( hash_table == NULL )
    {
        rc = -1;
        goto ErrExit;
    }
    
    hash_value = get_string_hash_value( string, length );
    hash_value = hash_value % hash_table->hash_size;
    hash_entry = &hash_table->entry[hash_value];
    
    if ( lstCount( hash_entry ) > 0 )
    {        
        for ( node = lstFirst(hash_entry);
              node != NULL;
              node = lstNext( node) )
        {
			hash_node = (HASH_STRING_NODE *)BYOFFSET( node, HASH_STRING_NODE, hash_node.entry_list_node );
            if ( hash_node->length == length &&
                 strncasecmp( hash_node->string, string, length) == 0 )
            {
                lstDelete( hash_entry, node );
                lstDelete( &hash_table->all, &hash_node->hash_node.all_list_node );
                rc = 0;
                break;
            }
			hash_node = NULL;
        }
    }
    
 ErrExit:    
    //FUNC_EXIT( rc );
    return  hash_node;
}



static UINT32 get_string_hash_value( char *string, int length )
{
    int     i;
    UINT32  hash_value = 0;
    
    for ( i=0; i<length; i++ )
    {
        hash_value += toupper( string[i] );
    }

    return hash_value;
}





RET_CODE hash_int_add( HASH           *hash_table,
                       HASH_INT_NODE  *node ) 
{
    
    int         rc = 0;
    UINT32      hash_value;
    PS_LIST    *hash_entry;
    HASH_INT_NODE     *query_cb;

    //FUNC_ENT();
    
    if ( hash_table == NULL )
    {
        rc = -1;
        goto ErrExit;
    }
    
    query_cb = hash_int_query( hash_table,
                               node->key );
    if ( query_cb )
    {
        // the transaction ID is already in there
        rc = -2;
        goto ErrExit;
    }
    
    
    hash_value = node->key % hash_table->hash_size;
    hash_entry = &hash_table->entry[hash_value];
    lstAdd( hash_entry, &node->hash_node.entry_list_node );
    lstAdd( &hash_table->all, &node->hash_node.all_list_node );

ErrExit:

	return rc;

}

HASH_INT_NODE* hash_int_query( HASH              *hash_table,
                               UINT32             key )
{

    UINT32      hash_value;
    PS_LIST    *hash_entry;
    HASH_INT_NODE  *hash_node;
	NODE       *node;
    
    if ( hash_table == NULL )
    {
        return NULL;
    }

    hash_value = key % hash_table->hash_size;
    hash_entry = &hash_table->entry[hash_value];

    if ( lstCount( hash_entry ) > 0 )
    {
        for ( node = lstFirst(hash_entry);
              node != NULL;
              node = lstNext( node) )
        {
			hash_node = (HASH_INT_NODE*)BYOFFSET( node, HASH_INT_NODE, hash_node.entry_list_node );
            if ( hash_node->key == key )
            {
                return  hash_node;
            }
        }
    }
    return NULL;
        
}


HASH_INT_NODE *hash_int_delete( HASH              *hash_table,
						        UINT32             key )
{
    

    UINT32             hash_value;
    PS_LIST           *hash_entry;
    HASH_INT_NODE     *hash_node = NULL;
	NODE              *node; 
    int                rc = 1;  // in case not found any 
    
    //FUNC_ENT();
    
    if ( hash_table == NULL )
    {
        rc = -1;
        goto ErrExit;
    }
    
    hash_value = key % hash_table->hash_size;
    hash_entry = &hash_table->entry[hash_value];
    
    if ( lstCount( hash_entry ) > 0 )
    {        
        for ( node = lstFirst(hash_entry);
              node != NULL;
              node = lstNext( node) )
        {
			hash_node = (HASH_INT_NODE *)BYOFFSET( node, HASH_INT_NODE, hash_node.entry_list_node );
            if ( hash_node->key == key )
            {
                lstDelete( hash_entry, node );
                lstDelete( &hash_table->all, &hash_node->hash_node.all_list_node);
                rc = 0;
                break;
            }
			hash_node = NULL;
        }
    }
    
 ErrExit:    
    //FUNC_EXIT( rc );
    return hash_node;
}



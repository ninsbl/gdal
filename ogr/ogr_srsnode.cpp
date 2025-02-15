/******************************************************************************
 *
 * Project:  OpenGIS Simple Features Reference Implementation
 * Purpose:  The OGR_SRSNode class.
 * Author:   Frank Warmerdam, warmerdam@pobox.com
 *
 ******************************************************************************
 * Copyright (c) 1999,  Les Technologies SoftMap Inc.
 * Copyright (c) 2010-2013, Even Rouault <even dot rouault at spatialys.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 ****************************************************************************/

#include "cpl_port.h"
#include "ogr_spatialref.h"

#include <cctype>
#include <cstddef>
#include <cstring>

#include "ogr_core.h"
#include "ogr_p.h"
#include "cpl_conv.h"
#include "cpl_error.h"
#include "cpl_string.h"


/************************************************************************/
/*                            OGR_SRSNode()                             */
/************************************************************************/

/**
 * Constructor.
 *
 * @param pszValueIn this optional parameter can be used to initialize
 * the value of the node upon creation.  If omitted the node will be created
 * with a value of "".  Newly created OGR_SRSNodes have no children.
 */

OGR_SRSNode::OGR_SRSNode( const char * pszValueIn ) :
    pszValue(CPLStrdup(pszValueIn)),
    papoChildNodes(nullptr),
    poParent(nullptr),
    nChildren(0)
{}

/************************************************************************/
/*                            ~OGR_SRSNode()                            */
/************************************************************************/

OGR_SRSNode::~OGR_SRSNode()

{
    CPLFree( pszValue );

    ClearChildren();
}

/************************************************************************/
/*                             ~Listener()                              */
/************************************************************************/

OGR_SRSNode::Listener::~Listener() = default;

/************************************************************************/
/*                           RegisterListener()                         */
/************************************************************************/

void OGR_SRSNode::RegisterListener(const std::shared_ptr<Listener>& listener)
{
    m_listener = listener;
}

/************************************************************************/
/*                             notifyChange()                           */
/************************************************************************/

void OGR_SRSNode::notifyChange()
{
    auto locked = m_listener.lock();
    if( locked )
    {
        locked->notifyChange(this);
    }
}

/************************************************************************/
/*                           ClearChildren()                            */
/************************************************************************/

/** Clear children nodes
 */
void OGR_SRSNode::ClearChildren()

{
    for( int i = 0; i < nChildren; i++ )
    {
        delete papoChildNodes[i];
    }

    CPLFree( papoChildNodes );

    papoChildNodes = nullptr;
    nChildren = 0;
}

/************************************************************************/
/*                           GetChildCount()                            */
/************************************************************************/

/**
 * \fn int OGR_SRSNode::GetChildCount() const;
 *
 * Get number of children nodes.
 *
 * @return 0 for leaf nodes, or the number of children nodes.
 */

/************************************************************************/
/*                              GetChild()                              */
/************************************************************************/

/**
 * Fetch requested child.
 *
 * @param iChild the index of the child to fetch, from 0 to
 * GetChildCount() - 1.
 *
 * @return a pointer to the child OGR_SRSNode, or NULL if there is no such
 * child.
 */

OGR_SRSNode *OGR_SRSNode::GetChild( int iChild )

{
    if( iChild < 0 || iChild >= nChildren )
        return nullptr;

    return papoChildNodes[iChild];
}

/**
 * Fetch requested child.
 *
 * @param iChild the index of the child to fetch, from 0 to
 * GetChildCount() - 1.
 *
 * @return a pointer to the child OGR_SRSNode, or NULL if there is no such
 * child.
 */

const OGR_SRSNode *OGR_SRSNode::GetChild( int iChild ) const

{
    if( iChild < 0 || iChild >= nChildren )
        return nullptr;

    return papoChildNodes[iChild];
}

/************************************************************************/
/*                              GetNode()                               */
/************************************************************************/

/**
 * Find named node in tree.
 *
 * This method does a pre-order traversal of the node tree searching for
 * a node with this exact value (case insensitive), and returns it.  Leaf
 * nodes are not considered, under the assumption that they are just
 * attribute value nodes.
 *
 * If a node appears more than once in the tree (such as UNIT for instance),
 * the first encountered will be returned.  Use GetNode() on a subtree to be
 * more specific.
 *
 * @param pszName the name of the node to search for.
 *
 * @return a pointer to the node found, or NULL if none.
 */

OGR_SRSNode *OGR_SRSNode::GetNode( const char * pszName )

{
    if( nChildren > 0 && EQUAL(pszName, pszValue) )
        return this;

/* -------------------------------------------------------------------- */
/*      First we check the immediate children so we will get an         */
/*      immediate child in preference to a subchild.                    */
/* -------------------------------------------------------------------- */
    for( int i = 0; i < nChildren; i++ )
    {
        if( EQUAL(papoChildNodes[i]->pszValue, pszName)
            && papoChildNodes[i]->nChildren > 0 )
            return papoChildNodes[i];
    }

/* -------------------------------------------------------------------- */
/*      Then get each child to check their children.                    */
/* -------------------------------------------------------------------- */
    for( int i = 0; i < nChildren; i++ )
    {
        OGR_SRSNode *poNode = papoChildNodes[i]->GetNode( pszName );
        if( poNode != nullptr )
            return poNode;
    }

    return nullptr;
}

/**
 * Find named node in tree.
 *
 * This method does a pre-order traversal of the node tree searching for
 * a node with this exact value (case insensitive), and returns it.  Leaf
 * nodes are not considered, under the assumption that they are just
 * attribute value nodes.
 *
 * If a node appears more than once in the tree (such as UNIT for instance),
 * the first encountered will be returned.  Use GetNode() on a subtree to be
 * more specific.
 *
 * @param pszName the name of the node to search for.
 *
 * @return a pointer to the node found, or NULL if none.
 */

const OGR_SRSNode *OGR_SRSNode::GetNode( const char * pszName ) const

{
    return const_cast<OGR_SRSNode *>(this)->GetNode( pszName );
}

/************************************************************************/
/*                              AddChild()                              */
/************************************************************************/

/**
 * Add passed node as a child of target node.
 *
 * Note that ownership of the passed node is assumed by the node on which
 * the method is invoked ... use the Clone() method if the original is to
 * be preserved.  New children are always added at the end of the list.
 *
 * @param poNew the node to add as a child.
 */

void OGR_SRSNode::AddChild( OGR_SRSNode * poNew )

{
    InsertChild( poNew, nChildren );
}

/************************************************************************/
/*                            InsertChild()                             */
/************************************************************************/

/**
 * Insert the passed node as a child of target node, at the indicated
 * position.
 *
 * Note that ownership of the passed node is assumed by the node on which
 * the method is invoked ... use the Clone() method if the original is to
 * be preserved.  All existing children at location iChild and beyond are
 * push down one space to make space for the new child.
 *
 * @param poNew the node to add as a child.
 * @param iChild position to insert, use 0 to insert at the beginning.
 */

void OGR_SRSNode::InsertChild( OGR_SRSNode * poNew, int iChild )

{
    if( iChild > nChildren )
        iChild = nChildren;

    nChildren++;
    papoChildNodes = static_cast<OGR_SRSNode **>(
        CPLRealloc( papoChildNodes, sizeof(void*) * nChildren ) );

    memmove( papoChildNodes + iChild + 1, papoChildNodes + iChild,
             sizeof(void*) * (nChildren - iChild - 1) );

    papoChildNodes[iChild] = poNew;
    poNew->poParent = this;

    poNew->m_listener = m_listener;
    notifyChange();
}

/************************************************************************/
/*                            DestroyChild()                            */
/************************************************************************/

/**
 * Remove a child node, and it's subtree.
 *
 * Note that removing a child node will result in children after it
 * being renumbered down one.
 *
 * @param iChild the index of the child.
 */

void OGR_SRSNode::DestroyChild( int iChild )

{
    if( iChild < 0 || iChild >= nChildren )
        return;

    delete papoChildNodes[iChild];
    while( iChild < nChildren-1 )
    {
        papoChildNodes[iChild] = papoChildNodes[iChild+1];
        iChild++;
    }

    nChildren--;
    notifyChange();
}

/************************************************************************/
/*                             FindChild()                              */
/************************************************************************/

/**
 * Find the index of the child matching the given string.
 *
 * Note that the node value must match pszValue with the exception of
 * case.  The comparison is case insensitive.
 *
 * @param pszValueIn the node value being searched for.
 *
 * @return the child index, or -1 on failure.
 */

int OGR_SRSNode::FindChild( const char * pszValueIn ) const

{
    for( int i = 0; i < nChildren; i++ )
    {
        if( EQUAL(papoChildNodes[i]->pszValue, pszValueIn) )
            return i;
    }

    return -1;
}

/************************************************************************/
/*                              GetValue()                              */
/************************************************************************/

/**
 * \fn const char *OGR_SRSNode::GetValue() const;
 *
 * Fetch value string for this node.
 *
 * @return A non-NULL string is always returned.  The returned pointer is to
 * the internal value of this node, and should not be modified, or freed.
 */

/************************************************************************/
/*                              SetValue()                              */
/************************************************************************/

/**
 * Set the node value.
 *
 * @param pszNewValue the new value to assign to this node.  The passed
 * string is duplicated and remains the responsibility of the caller.
 */

void OGR_SRSNode::SetValue( const char * pszNewValue )

{
    CPLFree( pszValue );
    pszValue = CPLStrdup( pszNewValue );
    notifyChange();
}

/************************************************************************/
/*                               Clone()                                */
/************************************************************************/

/**
 * Make a duplicate of this node, and it's children.
 *
 * @return a new node tree, which becomes the responsibility of the caller.
 */

OGR_SRSNode *OGR_SRSNode::Clone() const

{
    OGR_SRSNode *poNew = new OGR_SRSNode( pszValue );

    for( int i = 0; i < nChildren; i++ )
    {
        poNew->AddChild( papoChildNodes[i]->Clone() );
    }
    poNew->m_listener = m_listener;

    return poNew;
}

/************************************************************************/
/*                            NeedsQuoting()                            */
/*                                                                      */
/*      Does this node need to be quoted when it is exported to Wkt?    */
/************************************************************************/

int OGR_SRSNode::NeedsQuoting() const

{
    // Non-terminals are never quoted.
    if( GetChildCount() != 0 )
        return FALSE;

    // As per bugzilla bug 201, the OGC spec says the authority code
    // needs to be quoted even though it appears well behaved.
    if( poParent != nullptr && EQUAL(poParent->GetValue(), "AUTHORITY") )
        return TRUE;

    // As per bugzilla bug 294, the OGC spec says the direction
    // values for the AXIS keywords should *not* be quoted.
    if( poParent != nullptr && EQUAL(poParent->GetValue(), "AXIS")
        && this != poParent->GetChild(0) )
        return FALSE;

    if( poParent != nullptr && EQUAL(poParent->GetValue(), "CS")
        && this == poParent->GetChild(0) )
        return FALSE;

    // Strings starting with e or E are not valid numeric values, so they
    // need quoting, like in AXIS["E",EAST]
    if( (pszValue[0] == 'e' || pszValue[0] == 'E') )
        return TRUE;

    // Non-numeric tokens are generally quoted while clean numeric values
    // are generally not.
    for( int i = 0; pszValue[i] != '\0'; i++ )
    {
        if( (pszValue[i] < '0' || pszValue[i] > '9')
            && pszValue[i] != '.'
            && pszValue[i] != '-' && pszValue[i] != '+'
            && pszValue[i] != 'e' && pszValue[i] != 'E' )
            return TRUE;
    }

    return FALSE;
}

/************************************************************************/
/*                            exportToWkt()                             */
/************************************************************************/

/**
 * Convert this tree of nodes into WKT format.
 *
 * Note that the returned WKT string should be freed with
 * CPLFree() when no longer needed.  It is the responsibility of the caller.
 *
 * @param ppszResult the resulting string is returned in this pointer.
 *
 * @return currently OGRERR_NONE is always returned, but the future it
 * is possible error conditions will develop.
 */

OGRErr OGR_SRSNode::exportToWkt( char ** ppszResult ) const

{
/* -------------------------------------------------------------------- */
/*      Build a list of the WKT format for the children.                */
/* -------------------------------------------------------------------- */
    char **papszChildrenWkt = static_cast<char **>(
        CPLCalloc(sizeof(char*), nChildren + 1) );
    size_t nLength = strlen(pszValue) + 4;

    for( int i = 0; i < nChildren; i++ )
    {
        papoChildNodes[i]->exportToWkt( papszChildrenWkt + i );
        nLength += strlen(papszChildrenWkt[i]) + 1;
    }

/* -------------------------------------------------------------------- */
/*      Allocate the result string.                                     */
/* -------------------------------------------------------------------- */
    *ppszResult = static_cast<char *>( CPLMalloc(nLength) );
    *ppszResult[0] = '\0';

/* -------------------------------------------------------------------- */
/*      Capture this nodes value.  We put it in double quotes if        */
/*      this is a leaf node, otherwise we assume it is a well formed    */
/*      node name.                                                      */
/* -------------------------------------------------------------------- */
    if( NeedsQuoting() )
    {
        strcat( *ppszResult, "\"" );
        strcat( *ppszResult, pszValue );  // Should we do quoting?
        strcat( *ppszResult, "\"" );
    }
    else
        strcat( *ppszResult, pszValue );

/* -------------------------------------------------------------------- */
/*      Add the children strings with appropriate brackets and commas.  */
/* -------------------------------------------------------------------- */
    if( nChildren > 0 )
        strcat( *ppszResult, "[" );

    for( int i = 0; i < nChildren; i++ )
    {
        strcat( *ppszResult, papszChildrenWkt[i] );
        if( i == nChildren - 1 )
            strcat( *ppszResult, "]" );
        else
            strcat( *ppszResult, "," );
    }

    CSLDestroy( papszChildrenWkt );

    return OGRERR_NONE;
}

/************************************************************************/
/*                         exportToPrettyWkt()                          */
/************************************************************************/

/**
 * Convert this tree of nodes into pretty WKT format.
 *
 * Note that the returned WKT string should be freed with
 * CPLFree() when no longer needed.  It is the responsibility of the caller.
 *
 * @param ppszResult the resulting string is returned in this pointer.
 *
 * @param nDepth depth of the node
 *
 * @return currently OGRERR_NONE is always returned, but the future it
 * is possible error conditions will develop.
 */

OGRErr OGR_SRSNode::exportToPrettyWkt( char ** ppszResult, int nDepth ) const

{
/* -------------------------------------------------------------------- */
/*      Build a list of the WKT format for the children.                */
/* -------------------------------------------------------------------- */
    char **papszChildrenWkt = static_cast<char **>(
        CPLCalloc(sizeof(char*), nChildren + 1) );
    size_t nLength = strlen(pszValue) + 4;

    for( int i = 0; i < nChildren; i++ )
    {
        papoChildNodes[i]->exportToPrettyWkt( papszChildrenWkt + i,
                                              nDepth + 1);
        nLength += strlen(papszChildrenWkt[i]) + 2 + nDepth*4;
    }

/* -------------------------------------------------------------------- */
/*      Allocate the result string.                                     */
/* -------------------------------------------------------------------- */
    *ppszResult = static_cast<char *>( CPLMalloc(nLength) );
    *ppszResult[0] = '\0';

/* -------------------------------------------------------------------- */
/*      Capture this nodes value.  We put it in double quotes if        */
/*      this is a leaf node, otherwise we assume it is a well formed    */
/*      node name.                                                      */
/* -------------------------------------------------------------------- */
    if( NeedsQuoting() )
    {
        strcat( *ppszResult, "\"" );
        strcat( *ppszResult, pszValue );  // Should we do quoting?
        strcat( *ppszResult, "\"" );
    }
    else
    {
        strcat( *ppszResult, pszValue );
    }

/* -------------------------------------------------------------------- */
/*      Add the children strings with appropriate brackets and commas.  */
/* -------------------------------------------------------------------- */
    if( nChildren > 0 )
        strcat( *ppszResult, "[" );

    for( int i = 0; i < nChildren; i++ )
    {
        if( papoChildNodes[i]->GetChildCount() > 0 )
        {
            strcat( *ppszResult, "\n" );
            for( int j = 0; j < 4*nDepth; j++ )
                strcat( *ppszResult, " " );
        }
        strcat( *ppszResult, papszChildrenWkt[i] );
        if( i < nChildren-1 )
            strcat( *ppszResult, "," );
    }

    if( nChildren > 0 )
    {
        if( (*ppszResult)[strlen(*ppszResult)-1] == ',' )
            (*ppszResult)[strlen(*ppszResult)-1] = '\0';

        strcat( *ppszResult, "]" );
    }

    CSLDestroy( papszChildrenWkt );

    return OGRERR_NONE;
}

/************************************************************************/
/*                           importFromWkt()                            */
/************************************************************************/

/**
 * Import from WKT string.
 *
 * This method will wipe the existing children and value of this node, and
 * reassign them based on the contents of the passed WKT string.  Only as
 * much of the input string as needed to construct this node, and its
 * children is consumed from the input string, and the input string pointer
 * is then updated to point to the remaining (unused) input.
 *
 * @param ppszInput Pointer to pointer to input.  The pointer is updated to
 * point to remaining unused input text.
 *
 * @return OGRERR_NONE if import succeeds, or OGRERR_CORRUPT_DATA if it
 * fails for any reason.
 * @deprecated GDAL 2.3. Use importFromWkt(const char**) instead.
 */

OGRErr OGR_SRSNode::importFromWkt( char ** ppszInput )

{
    int nNodes = 0;
    return importFromWkt( const_cast<const char**>(ppszInput), 0, &nNodes );
}

/**
 * Import from WKT string.
 *
 * This method will wipe the existing children and value of this node, and
 * reassign them based on the contents of the passed WKT string.  Only as
 * much of the input string as needed to construct this node, and its
 * children is consumed from the input string, and the input string pointer
 * is then updated to point to the remaining (unused) input.
 *
 * @param ppszInput Pointer to pointer to input.  The pointer is updated to
 * point to remaining unused input text.
 *
 * @return OGRERR_NONE if import succeeds, or OGRERR_CORRUPT_DATA if it
 * fails for any reason.
 *
 * @since GDAL 2.3
 */

OGRErr OGR_SRSNode::importFromWkt( const char ** ppszInput )

{
    int nNodes = 0;
    return importFromWkt( ppszInput, 0, &nNodes );
}

OGRErr OGR_SRSNode::importFromWkt( const char **ppszInput, int nRecLevel,
                                   int* pnNodes )

{
    // Sanity checks.
    if( nRecLevel == 10 )
    {
        return OGRERR_CORRUPT_DATA;
    }
    if( *pnNodes == 1000 )
    {
        return OGRERR_CORRUPT_DATA;
    }

    const char *pszInput = *ppszInput;
    bool bInQuotedString = false;

/* -------------------------------------------------------------------- */
/*      Clear any existing children of this node.                       */
/* -------------------------------------------------------------------- */
    ClearChildren();

/* -------------------------------------------------------------------- */
/*      Read the ``value'' for this node.                               */
/* -------------------------------------------------------------------- */
    {
        char szToken[512]; // do not initialize whole buffer. significant overhead
        size_t nTokenLen = 0;
        szToken[0] = '\0';

        while( *pszInput != '\0' &&
               nTokenLen + 1 < sizeof(szToken) )
        {
            if( *pszInput == '"' )
            {
                bInQuotedString = !bInQuotedString;
            }
            else if( !bInQuotedString
                  && (*pszInput == '[' || *pszInput == ']' || *pszInput == ','
                      || *pszInput == '(' || *pszInput == ')' ) )
            {
                break;
            }
            else if( !bInQuotedString
                     && (*pszInput == ' ' || *pszInput == '\t'
                         || *pszInput == 10 || *pszInput == 13) )
            {
                // Skip over whitespace.
            }
            else
            {
                szToken[nTokenLen++] = *pszInput;
            }

            pszInput++;
        }

        if( *pszInput == '\0' || nTokenLen == sizeof(szToken) - 1 )
            return OGRERR_CORRUPT_DATA;

        szToken[nTokenLen++] = '\0';
        SetValue( szToken );
    }

/* -------------------------------------------------------------------- */
/*      Read children, if we have a sublist.                            */
/* -------------------------------------------------------------------- */
    if( *pszInput == '[' || *pszInput == '(' )
    {
        do
        {
            pszInput++; // Skip bracket or comma.

            OGR_SRSNode *poNewChild = new OGR_SRSNode();
            poNewChild->m_listener = m_listener;

            (*pnNodes)++;
            const OGRErr eErr =
                poNewChild->importFromWkt(
                    &pszInput,
                    nRecLevel + 1, pnNodes );
            if( eErr != OGRERR_NONE )
            {
                delete poNewChild;
                return eErr;
            }

            AddChild( poNewChild );

            // Swallow whitespace.
            while( isspace(*pszInput) )
                pszInput++;
        } while( *pszInput == ',' );

        if( *pszInput != ')' && *pszInput != ']' )
            return OGRERR_CORRUPT_DATA;

        pszInput++;
    }

    *ppszInput = pszInput;

    return OGRERR_NONE;
}

/************************************************************************/
/*                           MakeValueSafe()                            */
/************************************************************************/

/**
 * Massage value string, stripping special characters so it will be a
 * database safe string.
 *
 * The operation is also applies to all subnodes of the current node.
 */

void OGR_SRSNode::MakeValueSafe()

{
/* -------------------------------------------------------------------- */
/*      First process subnodes.                                         */
/* -------------------------------------------------------------------- */
    for( int iChild = 0; iChild < GetChildCount(); iChild++ )
    {
        GetChild(iChild)->MakeValueSafe();
    }

/* -------------------------------------------------------------------- */
/*      Skip numeric nodes.                                             */
/* -------------------------------------------------------------------- */
    if( (pszValue[0] >= '0' && pszValue[0] <= '9') || pszValue[0] != '.' )
        return;

/* -------------------------------------------------------------------- */
/*      Translate non-alphanumeric values to underscores.               */
/* -------------------------------------------------------------------- */
    for( int i = 0; pszValue[i] != '\0'; i++ )
    {
        if( !(pszValue[i] >= 'A' && pszValue[i] <= 'Z')
            && !(pszValue[i] >= 'a' && pszValue[i] <= 'z')
            && !(pszValue[i] >= '0' && pszValue[i] <= '9') )
        {
            pszValue[i] = '_';
        }
    }

/* -------------------------------------------------------------------- */
/*      Remove repeated and trailing underscores.                       */
/* -------------------------------------------------------------------- */
    int j = 0;
    for( int i = 1; pszValue[i] != '\0'; i++ )
    {
        if( pszValue[j] == '_' && pszValue[i] == '_' )
            continue;

        pszValue[++j] = pszValue[i];
    }

    if( pszValue[j] == '_' )
        pszValue[j] = '\0';
    else
        pszValue[j+1] = '\0';
}

/************************************************************************/
/*                             StripNodes()                             */
/************************************************************************/

/**
 * Strip child nodes matching name.
 *
 * Removes any descendant nodes of this node that match the given name.
 * Of course children of removed nodes are also discarded.
 *
 * @param pszName the name for nodes that should be removed.
 */

void OGR_SRSNode::StripNodes( const char * pszName )

{
/* -------------------------------------------------------------------- */
/*      Strip any children matching this name.                          */
/* -------------------------------------------------------------------- */
    while( FindChild( pszName ) >= 0 )
        DestroyChild( FindChild( pszName ) );

/* -------------------------------------------------------------------- */
/*      Recurse                                                         */
/* -------------------------------------------------------------------- */
    for( int i = 0; i < GetChildCount(); i++ )
        GetChild(i)->StripNodes( pszName );
}


/* Begin user code block <abstract> */
/* End user code block <abstract> */

/**
 * README: Portions of this file are merged at file generation
 * time. Edits can be made *only* in between specified code blocks, look
 * for keywords <Begin user code> and <End user code>.
 */
/*
 * Generated by the ICS Builder Xcessory (BX).
 *
 * BuilderXcessory Version 6.1.3
 * Code Generator Xcessory 6.1.3 (08/19/04) CGX Scripts 6.1 Motif 2.1 
 *
 */
#ifndef MB3DNavList_H
#define MB3DNavList_H

/**
 * Forward declare the class data pointer type so that it can
 * easily be used as a parameter to class functions and data members.
 */
typedef struct _MB3DNavListData *MB3DNavListDataPtr;

/**
 * Globally included information.
 */


/**
 * Class specific includes.
 */


typedef struct _MB3DNavListData
{
    /*
     * Classes created by this class.
     */
    
    /*
     * Widgets created by this class.
     */
    Widget MB3DNavList;
    Widget mbview_navlist_label;
    Widget mbview_pushButton_navlist_delete;
    Widget mbview_pushButton_navlist_dismiss;
    Widget mbview_scrolledWindow_navlist;
    Widget mbview_list_navlist;
    
    /**
     * All methods and data..
     */
} MB3DNavListData;

/*
 * Function: MB3DNavListCreate()
 *		The creation for class MB3DNavList.
 */
MB3DNavListDataPtr MB3DNavListCreate(MB3DNavListDataPtr, Widget, String, ArgList, Cardinal);

/**
 * Set routines for exposed resources.
 */
#endif
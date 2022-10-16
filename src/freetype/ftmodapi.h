/***************************************************************************/
/*                                                                         */
/*  ftmodapi.h                                                             */
/*                                                                         */
/*    FreeType modules public interface (specification).                   */
/*                                                                         */
/*  Copyright 1996-2018 by                                                 */
/*  David Turner, Robert Wilhelm, and Werner Lemberg.                      */
/*                                                                         */
/*  This file is part of the FreeType project, and may only be used,       */
/*  modified, and distributed under the terms of the FreeType project      */
/*  license, LICENSE.TXT.  By continuing to use, modify, or distribute     */
/*  this file you indicate that you have read the license and              */
/*  understand and accept it fully.                                        */
/*                                                                         */
/***************************************************************************/


#ifndef FTMODAPI_H_
#define FTMODAPI_H_


#include "ft2build.h"
#include FT_FREETYPE_H


FT_BEGIN_HEADER


  /*************************************************************************/
  /*                                                                       */
  /* <Section>                                                             */
  /*    module_management                                                  */
  /*                                                                       */
  /* <Title>                                                               */
  /*    Module Management                                                  */
  /*                                                                       */
  /* <Abstract>                                                            */
  /*    How to add, upgrade, remove, and control modules from FreeType.    */
  /*                                                                       */
  /* <Description>                                                         */
  /*    The definitions below are used to manage modules within FreeType.  */
  /*    Modules can be added, upgraded, and removed at runtime.            */
  /*    Additionally, some module properties can be controlled also.       */
  /*                                                                       */
  /*    Here is a list of possible values of the `module_name' field in    */
  /*    the @FT_Module_Class structure.                                    */
  /*                                                                       */
  /*    {                                                                  */
  /*      autofitter                                                       */
  /*      bdf                                                              */
  /*      cff                                                              */
  /*      gxvalid                                                          */
  /*      otvalid                                                          */
  /*      pcf                                                              */
  /*      pfr                                                              */
  /*      psaux                                                            */
  /*      pshinter                                                         */
  /*      psnames                                                          */
  /*      sfnt                                                             */
  /*      smooth, smooth-lcd, smooth-lcdv                                  */
  /*      truetype                                                         */
  /*      type1                                                            */
  /*      type42                                                           */
  /*      t1cid                                                            */
  /*      winfonts                                                         */
  /*    }                                                                  */
  /*                                                                       */
  /*    Note that the FreeType Cache sub-system is not a FreeType module.  */
  /*                                                                       */
  /* <Order>                                                               */
  /*    FT_Module                                                          */
  /*    FT_Module_Constructor                                              */
  /*    FT_Module_Destructor                                               */
  /*    FT_Module_Requester                                                */
  /*    FT_Module_Class                                                    */
  /*                                                                       */
  /*    FT_Add_Module                                                      */
  /*    FT_Get_Module                                                      */
  /*    FT_Remove_Module                                                   */
  /*    FT_Add_Default_Modules                                             */
  /*                                                                       */
  /*    FT_New_Library                                                     */
  /*    FT_Done_Library                                                    */
  /*                                                                       */
  /*    FT_Renderer                                                        */
  /*    FT_Renderer_Class                                                  */
  /*                                                                       */
  /*    FT_Set_Debug_Hook                                                  */
  /*                                                                       */
  /*************************************************************************/


  /* module bit flags */
#define FT_MODULE_FONT_DRIVER         1  /* this module is a font driver  */
#define FT_MODULE_RENDERER            2  /* this module is a renderer     */
#define FT_MODULE_HINTER              4  /* this module is a glyph hinter */
#define FT_MODULE_STYLER              8  /* this module is a styler       */

#define FT_MODULE_DRIVER_SCALABLE      0x100  /* the driver supports      */
                                              /* scalable fonts           */
#define FT_MODULE_DRIVER_NO_OUTLINES   0x200  /* the driver does not      */
                                              /* support vector outlines  */
#define FT_MODULE_DRIVER_HAS_HINTER    0x400  /* the driver provides its  */
                                              /* own hinter               */
#define FT_MODULE_DRIVER_HINTS_LIGHTLY 0x800  /* the driver's hinter      */
                                              /* produces LIGHT hints     */



  typedef FT_Pointer  FT_Module_Interface;


  /*************************************************************************/
  /*                                                                       */
  /* <FuncType>                                                            */
  /*    FT_Module_Constructor                                              */
  /*                                                                       */
  /* <Description>                                                         */
  /*    A function used to initialize (not create) a new module object.    */
  /*                                                                       */
  /* <Input>                                                               */
  /*    module :: The module to initialize.                                */
  /*                                                                       */
  typedef FT_Error
  (*FT_Module_Constructor)( FT_Module  module );


  /*************************************************************************/
  /*                                                                       */
  /* <FuncType>                                                            */
  /*    FT_Module_Destructor                                               */
  /*                                                                       */
  /* <Description>                                                         */
  /*    A function used to finalize (not destroy) a given module object.   */
  /*                                                                       */
  /* <Input>                                                               */
  /*    module :: The module to finalize.                                  */
  /*                                                                       */
  typedef void
  (*FT_Module_Destructor)( FT_Module  module );


  /*************************************************************************/
  /*                                                                       */
  /* <FuncType>                                                            */
  /*    FT_Module_Requester                                                */
  /*                                                                       */
  /* <Description>                                                         */
  /*    A function used to query a given module for a specific interface.  */
  /*                                                                       */
  /* <Input>                                                               */
  /*    module :: The module to be searched.                               */
  /*                                                                       */
  /*    name ::   The name of the interface in the module.                 */
  /*                                                                       */
  typedef FT_Module_Interface
  (*FT_Module_Requester)( FT_Module    module,
                          const char*  name );


  /*************************************************************************/
  /*                                                                       */
  /* <Struct>                                                              */
  /*    FT_Module_Class                                                    */
  /*                                                                       */
  /* <Description>                                                         */
  /*    The module class descriptor.                                       */
  /*                                                                       */
  /* <Fields>                                                              */
  /*    module_flags    :: Bit flags describing the module.                */
  /*                                                                       */
  /*    module_size     :: The size of one module object/instance in       */
  /*                       bytes.                                          */
  /*                                                                       */
  /*    module_name     :: The name of the module.                         */
  /*                                                                       */
  /*    module_version  :: The version, as a 16.16 fixed number            */
  /*                       (major.minor).                                  */
  /*                                                                       */
  /*    module_requires :: The version of FreeType this module requires,   */
  /*                       as a 16.16 fixed number (major.minor).  Starts  */
  /*                       at version 2.0, i.e., 0x20000.                  */
  /*                                                                       */
  /*    module_init     :: The initializing function.                      */
  /*                                                                       */
  /*    module_done     :: The finalizing function.                        */
  /*                                                                       */
  /*    get_interface   :: The interface requesting function.              */
  /*                                                                       */
  typedef struct  FT_Module_Class_
  {
    FT_ULong               module_flags;
    FT_Long                module_size;
    const FT_String*       module_name;
    FT_Fixed               module_version;
    FT_Fixed               module_requires;

    const void*            module_interface;

    FT_Module_Constructor  module_init;
    FT_Module_Destructor   module_done;
    FT_Module_Requester    get_interface;

  } FT_Module_Class;


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    FT_Add_Module                                                      */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Add a new module to a given library instance.                      */
  /*                                                                       */
  /* <InOut>                                                               */
  /*    library :: A handle to the library object.                         */
  /*                                                                       */
  /* <Input>                                                               */
  /*    clazz   :: A pointer to class descriptor for the module.           */
  /*                                                                       */
  /* <Return>                                                              */
  /*    FreeType error code.  0~means success.                             */
  /*                                                                       */
  /* <Note>                                                                */
  /*    An error will be returned if a module already exists by that name, */
  /*    or if the module requires a version of FreeType that is too great. */
  /*                                                                       */
  FT_EXPORT( FT_Error )
  FT_Add_Module( FT_Library              library,
                 const FT_Module_Class*  clazz );


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    FT_Get_Module                                                      */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Find a module by its name.                                         */
  /*                                                                       */
  /* <Input>                                                               */
  /*    library     :: A handle to the library object.                     */
  /*                                                                       */
  /*    module_name :: The module's name (as an ASCII string).             */
  /*                                                                       */
  /* <Return>                                                              */
  /*    A module handle.  0~if none was found.                             */
  /*                                                                       */
  /* <Note>                                                                */
  /*    FreeType's internal modules aren't documented very well, and you   */
  /*    should look up the source code for details.                        */
  /*                                                                       */
  FT_EXPORT( FT_Module )
  FT_Get_Module( FT_Library   library,
                 const char*  module_name );


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    FT_Remove_Module                                                   */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Remove a given module from a library instance.                     */
  /*                                                                       */
  /* <InOut>                                                               */
  /*    library :: A handle to a library object.                           */
  /*                                                                       */
  /* <Input>                                                               */
  /*    module  :: A handle to a module object.                            */
  /*                                                                       */
  /* <Return>                                                              */
  /*    FreeType error code.  0~means success.                             */
  /*                                                                       */
  /* <Note>                                                                */
  /*    The module object is destroyed by the function in case of success. */
  /*                                                                       */
  FT_EXPORT( FT_Error )
  FT_Remove_Module( FT_Library  library,
                    FT_Module   module );


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    FT_New_Library                                                     */
  /*                                                                       */
  /* <Description>                                                         */
  /*    This function is used to create a new FreeType library instance    */
  /*    from a given memory object.  It is thus possible to use libraries  */
  /*    with distinct memory allocators within the same program.  Note,    */
  /*    however, that the used @FT_Memory structure is expected to remain  */
  /*    valid for the life of the @FT_Library object.                      */
  /*                                                                       */
  /*    Normally, you would call this function (followed by a call to      */
  /*    @FT_Add_Default_Modules or a series of calls to @FT_Add_Module,    */
  /*    and a call to @FT_Set_Default_Properties) instead of               */
  /*    @FT_Init_FreeType to initialize the FreeType library.              */
  /*                                                                       */
  /*    Don't use @FT_Done_FreeType but @FT_Done_Library to destroy a      */
  /*    library instance.                                                  */
  /*                                                                       */
  /* <Input>                                                               */
  /*    memory   :: A handle to the original memory object.                */
  /*                                                                       */
  /* <Output>                                                              */
  /*    alibrary :: A pointer to handle of a new library object.           */
  /*                                                                       */
  /* <Return>                                                              */
  /*    FreeType error code.  0~means success.                             */
  /*                                                                       */
  FT_EXPORT( FT_Error )
  FT_New_Library( FT_Memory    memory,
                  FT_Library  *alibrary );


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    FT_Done_Library                                                    */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Discard a given library object.  This closes all drivers and       */
  /*    discards all resource objects.                                     */
  /*                                                                       */
  /* <Input>                                                               */
  /*    library :: A handle to the target library.                         */
  /*                                                                       */
  /* <Return>                                                              */
  /*    FreeType error code.  0~means success.                             */
  /*                                                                       */
  FT_EXPORT( FT_Error )
  FT_Done_Library( FT_Library  library );


  /*************************************************************************/
  /*                                                                       */
  /* <Function>                                                            */
  /*    FT_Add_Default_Modules                                             */
  /*                                                                       */
  /* <Description>                                                         */
  /*    Add the set of default drivers to a given library object.          */
  /*    This is only useful when you create a library object with          */
  /*    @FT_New_Library (usually to plug a custom memory manager).         */
  /*                                                                       */
  /* <InOut>                                                               */
  /*    library :: A handle to a new library object.                       */
  /*                                                                       */
  FT_EXPORT( void )
  FT_Add_Default_Modules( FT_Library  library );

  /* */


FT_END_HEADER

#endif /* FTMODAPI_H_ */


/* END */

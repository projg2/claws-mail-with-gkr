/* Automatically generated file.  Do not edit directly. */

/* This file is part of The New Aspell
 * Copyright (C) 2001-2002 by Kevin Atkinson under the GNU LGPL
 * license version 2.0 or 2.1.  You should have received a copy of the
 * LGPL license along with this library if you did not you can find it
 * at http://www.gnu.org/.                                              */

#ifndef W32_ASPELL_INIT_H
#define W32_ASPELL_INIT_H

#ifdef __cplusplus
extern "C" {
#endif

#include <windows.h>

int w32_init_aspell(void);
int w32_aspell_loaded(void);

#define W32_ASPELL_DECLARE(nam) t##nam nam ;

/******************************* type id *******************************/


union AspellTypeId {

  unsigned int num;

  char str[4];

};


typedef union AspellTypeId AspellTypeId;


/************************** mutable container **************************/


typedef struct AspellMutableContainer AspellMutableContainer;


typedef int (__cdecl * taspell_mutable_container_add)(struct AspellMutableContainer * ths, const char * to_add);

typedef int (__cdecl * taspell_mutable_container_remove)(struct AspellMutableContainer * ths, const char * to_rem);

typedef void (__cdecl * taspell_mutable_container_clear)(struct AspellMutableContainer * ths);

// typedef struct AspellMutableContainer * (__cdecl * taspell_mutable_container_to_mutable_container)(struct AspellMutableContainer * ths); */



/******************************* key info *******************************/



enum AspellKeyInfoType {AspellKeyInfoString, AspellKeyInfoInt, AspellKeyInfoBool, AspellKeyInfoList};
typedef enum AspellKeyInfoType AspellKeyInfoType;


struct AspellKeyInfo {

  /* the name of the key */
  const char * name;

  /* the key type */
  enum AspellKeyInfoType type;

  /* the default value of the key */
  const char * def;

  /* a brief description of the key or null if internal value */
  const char * desc;

  /* other data used by config implementations
   * should be set to 0 if not used */
  char otherdata[16];

};


typedef struct AspellKeyInfo AspellKeyInfo;


/******************************** config ********************************/


typedef struct AspellKeyInfoEnumeration AspellKeyInfoEnumeration;


int aspell_key_info_enumeration_at_end(const struct AspellKeyInfoEnumeration * ths);

typedef const struct AspellKeyInfo * (__cdecl * taspell_key_info_enumeration_next)(struct AspellKeyInfoEnumeration * ths);

typedef void (__cdecl * tdelete_aspell_key_info_enumeration)(struct AspellKeyInfoEnumeration * ths);

typedef struct AspellKeyInfoEnumeration * (__cdecl * taspell_key_info_enumeration_clone)(const struct AspellKeyInfoEnumeration * ths);

typedef void (__cdecl * taspell_key_info_enumeration_assign)(struct AspellKeyInfoEnumeration * ths, const struct AspellKeyInfoEnumeration * other);



typedef struct AspellConfig AspellConfig;


typedef struct AspellConfig * (__cdecl * tnew_aspell_config)();

typedef void (__cdecl * tdelete_aspell_config)(struct AspellConfig * ths);

typedef struct AspellConfig * (__cdecl * taspell_config_clone)(const struct AspellConfig * ths);

typedef void (__cdecl * taspell_config_assign)(struct AspellConfig * ths, const struct AspellConfig * other);

typedef unsigned int (__cdecl * taspell_config_error_number)(const struct AspellConfig * ths);

typedef const char * (__cdecl * taspell_config_error_message)(const struct AspellConfig * ths);

typedef const struct AspellError * (__cdecl * taspell_config_error)(const struct AspellConfig * ths);

/* sets extra keys which this config class should accept
 * begin and end are expected to point to the begging
 * and end of an array of Aspell Key Info */
typedef void (__cdecl * taspell_config_set_extra)(struct AspellConfig * ths, const struct AspellKeyInfo * begin, const struct AspellKeyInfo * end);

/* returns the KeyInfo object for the
 * corresponding key or returns null and sets
 * error_num to PERROR_UNKNOWN_KEY if the key is
 * not valid. The pointer returned is valid for
 * the lifetime of the object. */
typedef const struct AspellKeyInfo * (__cdecl * taspell_config_keyinfo)(struct AspellConfig * ths, const char * key);

/* returns a newly allocated enumeration of all the
 * possible objects this config class uses */
typedef struct AspellKeyInfoEnumeration * (__cdecl * taspell_config_possible_elements)(struct AspellConfig * ths, int include_extra);

/* returns the default value for given key which
 * way involve substating variables, thus it is
 * not the same as keyinfo(key)->def returns null
 * and sets error_num to PERROR_UNKNOWN_KEY if
 * the key is not valid. Uses the temporary
 * string. */
typedef const char * (__cdecl * taspell_config_get_default)(struct AspellConfig * ths, const char * key);

/* returns a newly alloacted enumeration of all the
 * key/value pairs. This DOES not include ones
 * which are set to their default values */
typedef struct AspellStringPairEnumeration * (__cdecl * taspell_config_elements)(struct AspellConfig * ths);

/* inserts an item, if the item already exists it
 * will be replaced. returns true if it succesed
 * or false on error. If the key in not valid it
 * sets error_num to PERROR_UNKNOWN_KEY, if the
 * value is not valid it will sets error_num to
 * PERROR_BAD_VALUE, if the value can not be
 * changed it sets error_num to
 * PERROR_CANT_CHANGE_VALUE, and if the value is
 * a list and you are trying to set it directory
 * it sets error_num to PERROR_LIST_SET */
typedef int (__cdecl * taspell_config_replace)(struct AspellConfig * ths, const char * key, const char * value);

/* remove a key and returns true if it exists
 * otherise return false. This effictly sets the
 * key to its default value. Calling replace with
 * a value of "<default>" will also call
 * remove. If the key does not exists sets
 * error_num to 0 or PERROR_NOT, if the key in
 * not valid sets error_num to
 * PERROR_UNKNOWN_KEY, if the value can not be
 * changed sets error_num to
 * PERROR_CANT_CHANGE_VALUE */
typedef int (__cdecl * taspell_config_remove)(struct AspellConfig * ths, const char * key);

typedef int (__cdecl * taspell_config_have)(const struct AspellConfig * ths, const char * key);

/* returns null on error */
typedef const char * (__cdecl * taspell_config_retrieve)(struct AspellConfig * ths, const char * key);

typedef int (__cdecl * taspell_config_retrieve_list)(struct AspellConfig * ths, const char * key, struct AspellMutableContainer * lst);

/* return -1 on error, 0 if false, 1 if true */
typedef int (__cdecl * taspell_config_retrieve_bool)(struct AspellConfig * ths, const char * key);

/* return -1 on error */
typedef int (__cdecl * taspell_config_retrieve_int)(struct AspellConfig * ths, const char * key);



/******************************** error ********************************/


struct AspellError {

  const char * mesg;

  const struct AspellErrorInfo * err;

};


typedef struct AspellError AspellError;

typedef int (__cdecl * taspell_error_is_a)(const struct AspellError * ths, const struct AspellErrorInfo * e);


struct AspellErrorInfo {

  const struct AspellErrorInfo * isa;

  const char * mesg;

  unsigned int num_parms;

  const char * parms[3];

};


typedef struct AspellErrorInfo AspellErrorInfo;


/**************************** can have error ****************************/


typedef struct AspellCanHaveError AspellCanHaveError;


typedef unsigned int (__cdecl * taspell_error_number)(const struct AspellCanHaveError * ths);

typedef const char * (__cdecl * taspell_error_message)(const struct AspellCanHaveError * ths);

typedef const struct AspellError * (__cdecl * taspell_error)(const struct AspellCanHaveError * ths);

typedef void (__cdecl * tdelete_aspell_can_have_error)(struct AspellCanHaveError * ths);



/******************************** errors ********************************/


/* extern const struct AspellErrorInfo * const aerror_other; */
/* extern const struct AspellErrorInfo * const aerror_operation_not_supported; */
/* extern const struct AspellErrorInfo * const   aerror_cant_copy; */
/* extern const struct AspellErrorInfo * const aerror_file; */
/* extern const struct AspellErrorInfo * const   aerror_cant_open_file; */
/* extern const struct AspellErrorInfo * const     aerror_cant_read_file; */
/* extern const struct AspellErrorInfo * const     aerror_cant_write_file; */
/* extern const struct AspellErrorInfo * const   aerror_invalid_name; */
/* extern const struct AspellErrorInfo * const   aerror_bad_file_format; */
/* extern const struct AspellErrorInfo * const aerror_dir; */
/* extern const struct AspellErrorInfo * const   aerror_cant_read_dir; */
/* extern const struct AspellErrorInfo * const aerror_config; */
/* extern const struct AspellErrorInfo * const   aerror_unknown_key; */
/* extern const struct AspellErrorInfo * const   aerror_cant_change_value; */
/* extern const struct AspellErrorInfo * const   aerror_bad_key; */
/* extern const struct AspellErrorInfo * const   aerror_bad_value; */
/* extern const struct AspellErrorInfo * const   aerror_duplicate; */
/* extern const struct AspellErrorInfo * const aerror_language_related; */
/* extern const struct AspellErrorInfo * const   aerror_unknown_language; */
/* extern const struct AspellErrorInfo * const   aerror_unknown_soundslike; */
/* extern const struct AspellErrorInfo * const   aerror_language_not_supported; */
/* extern const struct AspellErrorInfo * const   aerror_no_wordlist_for_lang; */
/* extern const struct AspellErrorInfo * const   aerror_mismatched_language; */
/* extern const struct AspellErrorInfo * const aerror_encoding; */
/* extern const struct AspellErrorInfo * const   aerror_unknown_encoding; */
/* extern const struct AspellErrorInfo * const   aerror_encoding_not_supported; */
/* extern const struct AspellErrorInfo * const   aerror_conversion_not_supported; */
/* extern const struct AspellErrorInfo * const aerror_pipe; */
/* extern const struct AspellErrorInfo * const   aerror_cant_create_pipe; */
/* extern const struct AspellErrorInfo * const   aerror_process_died; */
/* extern const struct AspellErrorInfo * const aerror_bad_input; */
/* extern const struct AspellErrorInfo * const   aerror_invalid_word; */
/* extern const struct AspellErrorInfo * const   aerror_word_list_flags; */
/* extern const struct AspellErrorInfo * const     aerror_invalid_flag; */
/* extern const struct AspellErrorInfo * const     aerror_conflicting_flags; */


/******************************* speller *******************************/


typedef struct AspellSpeller AspellSpeller;


typedef struct AspellCanHaveError * (__cdecl * tnew_aspell_speller)(struct AspellConfig * config);

typedef struct AspellSpeller * (__cdecl * tto_aspell_speller)(struct AspellCanHaveError * obj);

typedef void (__cdecl * tdelete_aspell_speller)(struct AspellSpeller * ths);

typedef unsigned int (__cdecl * taspell_speller_error_number)(const struct AspellSpeller * ths);

typedef const char * (__cdecl * taspell_speller_error_message)(const struct AspellSpeller * ths);

typedef const struct AspellError * (__cdecl * taspell_speller_error)(const struct AspellSpeller * ths);

typedef struct AspellConfig * (__cdecl * taspell_speller_config)(struct AspellSpeller * ths);

/* returns  0 if it is not in the dictionary,
 * 1 if it is, or -1 on error. */
typedef int (__cdecl * taspell_speller_check)(struct AspellSpeller * ths, const char * word, int word_size);

typedef int (__cdecl * taspell_speller_add_to_personal)(struct AspellSpeller * ths, const char * word, int word_size);

typedef int (__cdecl * taspell_speller_add_to_session)(struct AspellSpeller * ths, const char * word, int word_size);

typedef const struct AspellWordList * (__cdecl * taspell_speller_personal_word_list)(struct AspellSpeller * ths);

typedef const struct AspellWordList * (__cdecl * taspell_speller_session_word_list)(struct AspellSpeller * ths);

typedef const struct AspellWordList * (__cdecl * taspell_speller_main_word_list)(struct AspellSpeller * ths);

typedef int (__cdecl * taspell_speller_save_all_word_lists)(struct AspellSpeller * ths);

typedef int (__cdecl * taspell_speller_clear_session)(struct AspellSpeller * ths);

/* Return null on error.
 * the word list returned by suggest is only valid until the next
 * call to suggest */
typedef const struct AspellWordList * (__cdecl * taspell_speller_suggest)(struct AspellSpeller * ths, const char * word, int word_size);

typedef int (__cdecl * taspell_speller_store_replacement)(struct AspellSpeller * ths, const char * mis, int mis_size, const char * cor, int cor_size);



/******************************** filter ********************************/


typedef struct AspellFilter AspellFilter;


typedef void (__cdecl * tdelete_aspell_filter)(struct AspellFilter * ths);

typedef unsigned int (__cdecl * taspell_filter_error_number)(const struct AspellFilter * ths);

typedef const char * (__cdecl * taspell_filter_error_message)(const struct AspellFilter * ths);

typedef const struct AspellError * (__cdecl * taspell_filter_error)(const struct AspellFilter * ths);

typedef struct AspellFilter * (__cdecl * tto_aspell_filter)(struct AspellCanHaveError * obj);



/*************************** document checker ***************************/


struct AspellToken {

  unsigned int offset;

  unsigned int len;

};


typedef struct AspellToken AspellToken;


typedef struct AspellDocumentChecker AspellDocumentChecker;


typedef void (__cdecl * tdelete_aspell_document_checker)(struct AspellDocumentChecker * ths);

typedef unsigned int (__cdecl * taspell_document_checker_error_number)(const struct AspellDocumentChecker * ths);

typedef const char * (__cdecl * taspell_document_checker_error_message)(const struct AspellDocumentChecker * ths);

typedef const struct AspellError * (__cdecl * taspell_document_checker_error)(const struct AspellDocumentChecker * ths);

/* Creates a new document checker.
 * The speller class is expect to last until this
 * class is destroyed.
 * If config is given it will be used to overwide
 * any relevent options set by this speller class.
 * The config class is not once this function is done.
 * If filter is given then it will take ownership of
 * the filter class and use it to do the filtering.
 * You are expected to free the checker when done. */
typedef struct AspellCanHaveError * (__cdecl * tnew_aspell_document_checker)(struct AspellSpeller * speller);

typedef struct AspellDocumentChecker * (__cdecl * tto_aspell_document_checker)(struct AspellCanHaveError * obj);

/* reset the internal state of the filter.
 * should be called whenever a new document is being filtered */
typedef void (__cdecl * taspell_document_checker_reset)(struct AspellDocumentChecker * ths);

/* process a string
 * The string passed in should only be split on white space
 * characters.  Furthermore, between calles to reset, each string
 * should be passed in exactly once and in the order they appeared
 * in the document.  Passing in stings out of order, skipping
 * strings or passing them in more than once may lead to undefined
 * results. */
typedef void (__cdecl * taspell_document_checker_process)(struct AspellDocumentChecker * ths, const char * str, int size);

/* returns the next misspelled word in the processed string
 * if there are no more misspelled word than token.word
 * will be null and token.size will be 0 */
typedef struct AspellToken (__cdecl * taspell_document_checker_next_misspelling)(struct AspellDocumentChecker * ths);

/* returns the underlying filter class */
typedef struct AspellFilter * (__cdecl * taspell_document_checker_filter)(struct AspellDocumentChecker * ths);



/****************************** word list ******************************/


typedef struct AspellWordList AspellWordList;


typedef int (__cdecl * taspell_word_list_empty)(const struct AspellWordList * ths);

typedef unsigned int (__cdecl * taspell_word_list_size)(const struct AspellWordList * ths);

typedef struct AspellStringEnumeration * (__cdecl * taspell_word_list_elements)(const struct AspellWordList * ths);



/************************** string enumeration **************************/


typedef struct AspellStringEnumeration AspellStringEnumeration;


typedef void (__cdecl * tdelete_aspell_string_enumeration)(struct AspellStringEnumeration * ths);

typedef struct AspellStringEnumeration * (__cdecl * taspell_string_enumeration_clone)(const struct AspellStringEnumeration * ths);

typedef void (__cdecl * taspell_string_enumeration_assign)(struct AspellStringEnumeration * ths, const struct AspellStringEnumeration * other);

typedef int (__cdecl * taspell_string_enumeration_at_end)(const struct AspellStringEnumeration * ths);

typedef const char * (__cdecl * taspell_string_enumeration_next)(struct AspellStringEnumeration * ths);



/********************************* info *********************************/


struct AspellModuleInfo {

  const char * name;

  double order_num;

  const char * lib_dir;

  struct AspellStringList * dict_dirs;

  struct AspellStringList * dict_exts;

};


typedef struct AspellModuleInfo AspellModuleInfo;


struct AspellDictInfo {

  /* name to identify the dictionary by */
  const char * name;

  const char * code;

  const char * jargon;

  int size;

  const char * size_str;

  struct AspellModuleInfo * module;

};


typedef struct AspellDictInfo AspellDictInfo;


typedef struct AspellModuleInfoList AspellModuleInfoList;


typedef struct AspellModuleInfoList * (__cdecl * tget_aspell_module_info_list)(struct AspellConfig * config);

typedef int (__cdecl * taspell_module_info_list_empty)(const struct AspellModuleInfoList * ths);

typedef unsigned int (__cdecl * taspell_module_info_list_size)(const struct AspellModuleInfoList * ths);

typedef struct AspellModuleInfoEnumeration * (__cdecl * taspell_module_info_list_elements)(const struct AspellModuleInfoList * ths);



typedef struct AspellDictInfoList AspellDictInfoList;


typedef struct AspellDictInfoList * (__cdecl * tget_aspell_dict_info_list)(struct AspellConfig * config);

typedef int (__cdecl * taspell_dict_info_list_empty)(const struct AspellDictInfoList * ths);

typedef unsigned int (__cdecl * taspell_dict_info_list_size)(const struct AspellDictInfoList * ths);

typedef struct AspellDictInfoEnumeration * (__cdecl * taspell_dict_info_list_elements)(const struct AspellDictInfoList * ths);



typedef struct AspellModuleInfoEnumeration AspellModuleInfoEnumeration;


typedef int (__cdecl * taspell_module_info_enumeration_at_end)(const struct AspellModuleInfoEnumeration * ths);

typedef const struct AspellModuleInfo * (__cdecl * taspell_module_info_enumeration_next)(struct AspellModuleInfoEnumeration * ths);

typedef void (__cdecl * tdelete_aspell_module_info_enumeration)(struct AspellModuleInfoEnumeration * ths);

typedef struct AspellModuleInfoEnumeration * (__cdecl * taspell_module_info_enumeration_clone)(const struct AspellModuleInfoEnumeration * ths);

typedef void (__cdecl * taspell_module_info_enumeration_assign)(struct AspellModuleInfoEnumeration * ths, const struct AspellModuleInfoEnumeration * other);



typedef struct AspellDictInfoEnumeration AspellDictInfoEnumeration;


typedef int (__cdecl * taspell_dict_info_enumeration_at_end)(const struct AspellDictInfoEnumeration * ths);

typedef const struct AspellDictInfo * (__cdecl * taspell_dict_info_enumeration_next)(struct AspellDictInfoEnumeration * ths);

typedef void (__cdecl * tdelete_aspell_dict_info_enumeration)(struct AspellDictInfoEnumeration * ths);

typedef struct AspellDictInfoEnumeration * (__cdecl * taspell_dict_info_enumeration_clone)(const struct AspellDictInfoEnumeration * ths);

typedef void (__cdecl * taspell_dict_info_enumeration_assign)(struct AspellDictInfoEnumeration * ths, const struct AspellDictInfoEnumeration * other);



/***************************** string list *****************************/


typedef struct AspellStringList AspellStringList;


typedef struct AspellStringList * (__cdecl * tnew_aspell_string_list)();

typedef int (__cdecl * taspell_string_list_empty)(const struct AspellStringList * ths);

typedef unsigned int (__cdecl * taspell_string_list_size)(const struct AspellStringList * ths);

typedef struct AspellStringEnumeration * (__cdecl * taspell_string_list_elements)(const struct AspellStringList * ths);

typedef int (__cdecl * taspell_string_list_add)(struct AspellStringList * ths, const char * to_add);

typedef int (__cdecl * taspell_string_list_remove)(struct AspellStringList * ths, const char * to_rem);

typedef void (__cdecl * taspell_string_list_clear)(struct AspellStringList * ths);

typedef struct AspellMutableContainer * (__cdecl * taspell_string_list_to_mutable_container)(struct AspellStringList * ths);

typedef void (__cdecl * tdelete_aspell_string_list)(struct AspellStringList * ths);

typedef struct AspellStringList * (__cdecl * taspell_string_list_clone)(const struct AspellStringList * ths);

typedef void (__cdecl * taspell_string_list_assign)(struct AspellStringList * ths, const struct AspellStringList * other);



/****************************** string map ******************************/


typedef struct AspellStringMap AspellStringMap;


typedef struct AspellStringMap * (__cdecl * tnew_aspell_string_map)();

typedef int (__cdecl * taspell_string_map_add)(struct AspellStringMap * ths, const char * to_add);

typedef int (__cdecl * taspell_string_map_remove)(struct AspellStringMap * ths, const char * to_rem);

typedef void (__cdecl * taspell_string_map_clear)(struct AspellStringMap * ths);

typedef struct AspellMutableContainer * (__cdecl * taspell_string_map_to_mutable_container)(struct AspellStringMap * ths);

typedef void (__cdecl * tdelete_aspell_string_map)(struct AspellStringMap * ths);

typedef struct AspellStringMap * (__cdecl * taspell_string_map_clone)(const struct AspellStringMap * ths);

typedef void (__cdecl * taspell_string_map_assign)(struct AspellStringMap * ths, const struct AspellStringMap * other);

typedef int (__cdecl * taspell_string_map_empty)(const struct AspellStringMap * ths);

typedef unsigned int (__cdecl * taspell_string_map_size)(const struct AspellStringMap * ths);

typedef struct AspellStringPairEnumeration * (__cdecl * taspell_string_map_elements)(const struct AspellStringMap * ths);

/* Insert a new element.
 * Will NOT overright an existing entry.
 * Returns false if the element already exists. */
typedef int (__cdecl * taspell_string_map_insert)(struct AspellStringMap * ths, const char * key, const char * value);

/* Insert a new element.
 * Will overright an existing entry.
 * Always returns true. */
typedef int (__cdecl * taspell_string_map_replace)(struct AspellStringMap * ths, const char * key, const char * value);

/* Looks up an element.
 * Returns null if the element did not exist.
 * Returns an empty string if the element exists but has a null value.
 * Otherwises returns the value */
typedef const char * (__cdecl * taspell_string_map_lookup)(const struct AspellStringMap * ths, const char * key);



/***************************** string pair *****************************/


struct AspellStringPair {

  const char * first;

  const char * second;

};


typedef struct AspellStringPair AspellStringPair;


/*********************** string pair enumeration ***********************/


typedef struct AspellStringPairEnumeration AspellStringPairEnumeration;


typedef int (__cdecl * taspell_string_pair_enumeration_at_end)(const struct AspellStringPairEnumeration * ths);

typedef struct AspellStringPair (__cdecl * taspell_string_pair_enumeration_next)(struct AspellStringPairEnumeration * ths);

typedef void (__cdecl * tdelete_aspell_string_pair_enumeration)(struct AspellStringPairEnumeration * ths);

typedef struct AspellStringPairEnumeration * (__cdecl * taspell_string_pair_enumeration_clone)(const struct AspellStringPairEnumeration * ths);

typedef void (__cdecl * taspell_string_pair_enumeration_assign)(struct AspellStringPairEnumeration * ths, const struct AspellStringPairEnumeration * other);


/* typedefs */

W32_ASPELL_DECLARE( aspell_mutable_container_add                                          )
W32_ASPELL_DECLARE( aspell_mutable_container_remove                                       )
W32_ASPELL_DECLARE( aspell_mutable_container_clear                                        )
//W32_ASPELL_DECLARE( spell_mutable_container_to_mutable_container                          )
W32_ASPELL_DECLARE( aspell_key_info_enumeration_next                                      )
W32_ASPELL_DECLARE( delete_aspell_key_info_enumeration                                    )
W32_ASPELL_DECLARE( aspell_key_info_enumeration_clone                                     )
W32_ASPELL_DECLARE( aspell_key_info_enumeration_assign                                    )
W32_ASPELL_DECLARE( new_aspell_config                                                     )
W32_ASPELL_DECLARE( delete_aspell_config                                                  )
W32_ASPELL_DECLARE( aspell_config_clone                                                   )
W32_ASPELL_DECLARE( aspell_config_assign                                                  )
W32_ASPELL_DECLARE( aspell_config_error_number                                            )
W32_ASPELL_DECLARE( aspell_config_error_message                                           )
W32_ASPELL_DECLARE( aspell_config_error                                                   )
W32_ASPELL_DECLARE( aspell_config_set_extra                                               )
W32_ASPELL_DECLARE( aspell_config_keyinfo                                                 )
W32_ASPELL_DECLARE( aspell_config_possible_elements                                       )
W32_ASPELL_DECLARE( aspell_config_get_default                                             )
W32_ASPELL_DECLARE( aspell_config_elements                                                )
W32_ASPELL_DECLARE( aspell_config_replace                                                 )
W32_ASPELL_DECLARE( aspell_config_remove                                                  )
W32_ASPELL_DECLARE( aspell_config_have                                                    )
W32_ASPELL_DECLARE( aspell_config_retrieve                                                )
W32_ASPELL_DECLARE( aspell_config_retrieve_list                                           )
W32_ASPELL_DECLARE( aspell_config_retrieve_bool                                           )
W32_ASPELL_DECLARE( aspell_config_retrieve_int                                            )
W32_ASPELL_DECLARE( aspell_error_is_a                                                     )
W32_ASPELL_DECLARE( aspell_error_number                                                   )
W32_ASPELL_DECLARE( aspell_error_message                                                  )
W32_ASPELL_DECLARE( aspell_error                                                          )
W32_ASPELL_DECLARE( delete_aspell_can_have_error                                          )
W32_ASPELL_DECLARE( new_aspell_speller                                                    )
W32_ASPELL_DECLARE( to_aspell_speller                                                     )
W32_ASPELL_DECLARE( delete_aspell_speller                                                 )
W32_ASPELL_DECLARE( aspell_speller_error_number                                           )
W32_ASPELL_DECLARE( aspell_speller_error_message                                          )
W32_ASPELL_DECLARE( aspell_speller_error                                                  )
W32_ASPELL_DECLARE( aspell_speller_config                                                 )
W32_ASPELL_DECLARE( aspell_speller_check                                                  )
W32_ASPELL_DECLARE( aspell_speller_add_to_personal                                        )
W32_ASPELL_DECLARE( aspell_speller_add_to_session                                         )
W32_ASPELL_DECLARE( aspell_speller_personal_word_list                                     )
W32_ASPELL_DECLARE( aspell_speller_session_word_list                                      )
W32_ASPELL_DECLARE( aspell_speller_main_word_list                                         )
W32_ASPELL_DECLARE( aspell_speller_save_all_word_lists                                    )
W32_ASPELL_DECLARE( aspell_speller_clear_session                                          )
W32_ASPELL_DECLARE( aspell_speller_suggest                                                )
W32_ASPELL_DECLARE( aspell_speller_store_replacement                                      )
W32_ASPELL_DECLARE( delete_aspell_filter                                                  )
W32_ASPELL_DECLARE( aspell_filter_error_number                                            )
W32_ASPELL_DECLARE( aspell_filter_error_message                                           )
W32_ASPELL_DECLARE( aspell_filter_error                                                   )
W32_ASPELL_DECLARE( to_aspell_filter                                                      )
W32_ASPELL_DECLARE( delete_aspell_document_checker                                        )
W32_ASPELL_DECLARE( aspell_document_checker_error_number                                  )
W32_ASPELL_DECLARE( aspell_document_checker_error_message                                 )
W32_ASPELL_DECLARE( aspell_document_checker_error                                         )
W32_ASPELL_DECLARE( new_aspell_document_checker                                           )
W32_ASPELL_DECLARE( to_aspell_document_checker                                            )
W32_ASPELL_DECLARE( aspell_document_checker_reset                                         )
W32_ASPELL_DECLARE( aspell_document_checker_process                                       )
W32_ASPELL_DECLARE( aspell_document_checker_next_misspelling                              )
W32_ASPELL_DECLARE( aspell_document_checker_filter                                        )
W32_ASPELL_DECLARE( aspell_word_list_empty                                                )
W32_ASPELL_DECLARE( aspell_word_list_size                                                 )
W32_ASPELL_DECLARE( aspell_word_list_elements                                             )
W32_ASPELL_DECLARE( delete_aspell_string_enumeration                                      )
W32_ASPELL_DECLARE( aspell_string_enumeration_clone                                       )
W32_ASPELL_DECLARE( aspell_string_enumeration_assign                                      )
W32_ASPELL_DECLARE( aspell_string_enumeration_at_end                                      )
W32_ASPELL_DECLARE( aspell_string_enumeration_next                                        )
W32_ASPELL_DECLARE( get_aspell_module_info_list                                           )
W32_ASPELL_DECLARE( aspell_module_info_list_empty                                         )
W32_ASPELL_DECLARE( aspell_module_info_list_size                                          )
W32_ASPELL_DECLARE( aspell_module_info_list_elements                                      )
W32_ASPELL_DECLARE( get_aspell_dict_info_list                                             )
W32_ASPELL_DECLARE( aspell_dict_info_list_empty                                           )
W32_ASPELL_DECLARE( aspell_dict_info_list_size                                            )
W32_ASPELL_DECLARE( aspell_dict_info_list_elements                                        )
W32_ASPELL_DECLARE( aspell_module_info_enumeration_at_end                                 )
W32_ASPELL_DECLARE( aspell_module_info_enumeration_next                                   )
W32_ASPELL_DECLARE( delete_aspell_module_info_enumeration                                 )
W32_ASPELL_DECLARE( aspell_module_info_enumeration_clone                                  )
W32_ASPELL_DECLARE( aspell_module_info_enumeration_assign                                 )
W32_ASPELL_DECLARE( aspell_dict_info_enumeration_at_end                                   )
W32_ASPELL_DECLARE( aspell_dict_info_enumeration_next                                     )
W32_ASPELL_DECLARE( delete_aspell_dict_info_enumeration                                   )
W32_ASPELL_DECLARE( aspell_dict_info_enumeration_clone                                    )
W32_ASPELL_DECLARE( aspell_dict_info_enumeration_assign                                   )
W32_ASPELL_DECLARE( new_aspell_string_list                                                )
W32_ASPELL_DECLARE( aspell_string_list_empty                                              )
W32_ASPELL_DECLARE( aspell_string_list_size                                               )
W32_ASPELL_DECLARE( aspell_string_list_elements                                           )
W32_ASPELL_DECLARE( aspell_string_list_add                                                )
W32_ASPELL_DECLARE( aspell_string_list_remove                                             )
W32_ASPELL_DECLARE( aspell_string_list_clear                                              )
W32_ASPELL_DECLARE( aspell_string_list_to_mutable_container                               )
W32_ASPELL_DECLARE( delete_aspell_string_list                                             )
W32_ASPELL_DECLARE( aspell_string_list_clone                                              )
W32_ASPELL_DECLARE( aspell_string_list_assign                                             )
W32_ASPELL_DECLARE( new_aspell_string_map                                                 )
W32_ASPELL_DECLARE( aspell_string_map_add                                                 )
W32_ASPELL_DECLARE( aspell_string_map_remove                                              )
W32_ASPELL_DECLARE( aspell_string_map_clear                                               )
W32_ASPELL_DECLARE( aspell_string_map_to_mutable_container                                )
W32_ASPELL_DECLARE( delete_aspell_string_map                                              )
W32_ASPELL_DECLARE( aspell_string_map_clone                                               )
W32_ASPELL_DECLARE( aspell_string_map_assign                                              )
W32_ASPELL_DECLARE( aspell_string_map_empty                                               )
W32_ASPELL_DECLARE( aspell_string_map_size                                                )
W32_ASPELL_DECLARE( aspell_string_map_elements                                            )
W32_ASPELL_DECLARE( aspell_string_map_insert                                              )
W32_ASPELL_DECLARE( aspell_string_map_replace                                             )
W32_ASPELL_DECLARE( aspell_string_map_lookup                                              )
W32_ASPELL_DECLARE( aspell_string_pair_enumeration_at_end                                 )
W32_ASPELL_DECLARE( aspell_string_pair_enumeration_next                                   )
W32_ASPELL_DECLARE( delete_aspell_string_pair_enumeration                                 )
W32_ASPELL_DECLARE( aspell_string_pair_enumeration_clone                                  )
W32_ASPELL_DECLARE( aspell_string_pair_enumeration_assign                                 )

#ifdef __cplusplus
}
#endif
#endif /* W32_ASPELL_INIT_H */

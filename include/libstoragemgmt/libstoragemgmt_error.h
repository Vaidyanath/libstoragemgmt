/*
 * Copyright (C) 2011 Red Hat, Inc.
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301 USA
 *
 * Author: tasleson
 */

#ifndef LIBSTORAGEMGMTERROR_H
#define LIBSTORAGEMGMTERROR_H

#include <stdlib.h>

#ifdef  __cplusplus
extern "C" {
#endif

/**
 * Severity of the error.
 */
typedef enum  {
    LSM_ERR_LEVEL_NONE = 0,
    LSM_ERR_LEVEL_WARNING = 1,
    LSM_ERR_LEVEL_ERROR = 2
} lsmErrorLevel;

/*
 * Where the error took place
 */
typedef enum  {
    LSM_ERR_DOMAIN_FRAME_WORK = 0,
    LSM_ERR_DOMAIN_PLUG_IN = 1
} lsmErrorDomain;

/**
 * Possible enumerated return codes from library
 */
typedef enum {
    LSM_ERR_OK = 0,                     /**< OK */
    LSM_ERR_INTERNAL_ERROR = 1,         /**< Internal error */
    LSM_ERR_NO_MEMORY = 2,              /**< Memory allocation failure */
    LSM_ERR_NO_SUPPORT = 3,             /**< Feature not supported */
    LSM_ERR_UNKNOWN_HOST = 4,           /**< Name resolution failed */
    LSM_ERR_NO_CONNECT = 5,             /**< Unable to connect to host */
    LSM_ERR_INVALID_CONN = 6,           /**< Connection structure is invalid */
    LSM_ERR_JOB_STARTED = 7,            /**< Operation has started */
    LSM_ERR_INVALID_ARGUMENT = 8,       /**< Precondition checks failed */
    LSM_ERR_URI_PARSE = 9,              /**< Unable to parse URI */
    LSM_ERR_PLUGIN_PERMISSIONS = 10,    /**< Unable to access plugin */
    LSM_ERR_PLUGIN_DLOPEN = 11,         /**< dlopen on plugin failed */
    LSM_ERR_PLUGIN_DLSYM = 12,          /**< Required symbols in plugin missing */
    LSM_ERR_PLUGIN_ERROR = 13,          /**< Non-descript plugin error */
    LSM_ERR_INVALID_ERR = 14,           /**< Invalid error structure */
    LSM_ERR_PLUGIN_REGISTRATION = 15,   /**< Error during plug-in registration */
    LSM_ERR_INVALID_POOL = 16,          /**< Invalid pool pointer */
    LSM_ERR_INVALID_JOB_NUM = 17,       /**< Invalid job number */
    LSM_ERR_UNSUPPORTED_PROVISIONING = 18,      /**< Unsupported provisioning */
    LSM_ERR_INVALID_VOL = 19,           /**< Invalid job pointer */
    LSM_ERR_VOLUME_SAME_SIZE = 20,      /**< Trying to resize to same size */
    LSM_ERR_INVALID_INIT = 21,          /**< Invalid initiator structure */
    LSM_ERR_NO_MAPPING = 22,            /**< There is no access for initiator and volume */
    LSM_ERR_AUTH_FAILED = 45,           /**< Authorization failed */
} lsmErrorNumber;

typedef struct _lsmError lsmError;
typedef lsmError *lsmErrorPtr;

/**
 * Gets the last error structure
 * Note: @see lsmErrorFree to release memory
 * @param c      Connection pointer.
 * @return lsmErrorPtr, Null if no error exists!
 */
lsmErrorPtr lsmErrorGetLast(lsmConnectPtr c);

/**
 * Frees the error record!
 * @param err   The error to free!
 * @return LSM_ERR_OK on success, else error reason.
 */
int lsmErrorFree(lsmErrorPtr err);

/**
 * Retrieves the error number from the error.
 * @param e     The lsmErrorPtr
 * @return -1 if e is not a valid error pointer, else error number.
 */
lsmErrorNumber lsmErrorGetNumber(lsmErrorPtr e);

/**
 * Retrieves the domain from the error.
 * @param e     The lsmErrorPtr
 * @return -1 if e is not a valid error pointer, else error domain value.
 */
lsmErrorDomain lsmErrorGetDomain(lsmErrorPtr e);

/**
 * Retrieves the error level from the error.
 * @param e     The lsmErrorPtr
 * @return -1 if e is not a valid error pointer, else error level.
 */
lsmErrorLevel lsmErrorGetLevel(lsmErrorPtr e);

/**
 * Retrieves the error message from the error.
 * Note: The returned value is only valid as long as the e is valid, in addition
 * the function will return NULL if e is invalid.  To remove the ambiguity call
 * lsmErrorGetNumber and check return code.
 * @param e     The lsmErrorPtr
 * @return NULL if message data does not exist, else error message.
 */
char* lsmErrorGetMessage(lsmErrorPtr e);

/**
 * Retrieves the exception message from the error.
 * Note: The returned value is only valid as long as the e is valid, in addition
 * the function will return NULL if e is invalid.  To remove the ambiguity call
 * lsmErrorGetNumber and check return code.
 * @param e     The lsmErrorPtr
 * @return NULL if exception does not exist, else error exception.
 */
char* lsmErrorGetException(lsmErrorPtr e);

/**
 * Retrieves the error message from the error.
 * Note: The returned value is only valid as long as the e is valid, in addition
 * the function will return NULL if e is invalid.  To remove the ambiguity call
 * lsmErrorGetNumber and check return code.
 * @param e     The lsmErrorPtr
 * @return NULL if does not exist, else debug message.
 */
char* lsmErrorGetDebug(lsmErrorPtr e);

/**
 * Retrieves the debug data from the error.
 * Note: The returned value is only valid as long as the e is valid, in addition
 * the function will return NULL if e is invalid.  To remove the ambiguity call
 * lsmErrorGetNumber and check return code.
 * @param e             The lsmErrorPtr
 * @param[out] size     Number of bytes of data returned.
 * @return NULL if does not exist, else debug message.
 */
void* lsmErrorGetDebugData(lsmErrorPtr e, uint32_t *size);

#ifdef  __cplusplus
}
#endif

#endif  /* LIBSTORAGEMGMTERROR_H */

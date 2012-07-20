/*
 * Copyright (C) 2011-2012 Red Hat, Inc.
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * Author: tasleson
 *
 */

#include <libstoragemgmt/libstoragemgmt_plug_interface.h>
#include <string.h>
#include <inttypes.h>
#define _XOPEN_SOURCE
#include <unistd.h>
#include <crypt.h>
#include <glib.h>

#include "libstoragemgmt/libstoragemgmt_accessgroups.h"
#include "libstoragemgmt/libstoragemgmt_initiators.h"
#include "libstoragemgmt/libstoragemgmt_fs.h"
#include "libstoragemgmt/libstoragemgmt_snapshot.h"
#include "libstoragemgmt/libstoragemgmt_nfsexport.h"
#include "libstoragemgmt/libstoragemgmt_blockrange.h"
#include <time.h>

#ifdef  __cplusplus
extern "C" {
#endif

static char name[] = "Compiled plug-in example";
static char version [] = "0.01";
static char sys_id[] = "sim-01";

#define BS 512
#define MAX_SYSTEMS 1
#define MAX_POOLS 4
#define MAX_VOLUMES 128
#define MAX_FS 32
#define MAX_EXPORT 32

/**
 * Creates a pseudo md5 (DO NOT FREE RETURN VALUE!)
 * @param data      Data to generate md5
 * @return Pointer to character array.
 */
char* md5(const char *data)
{
	return crypt(data, "$1$LSM$");
}

/**
 * Removes an item from an array, shifting the elements and clearing the space
 * that was occupied at the end, use with caution :-)
 * @param array			Base address for the array
 * @param remove_index	Element index to remove
 * @param num_elems		Number of elements currently in the array
 * @param elem_size		Size of each array element
 */
void remove_item( void *array, int remove_index, int num_elems,
					size_t elem_size)
{
	if( array && (num_elems > 0) && (remove_index < num_elems) && elem_size ) {
		/*Are we at the end?, clear that which is at the end */
		if( remove_index + 1 == num_elems ) {
			memset(array + (elem_size * (num_elems - 1)), 0, elem_size);
			return;
		}

		/* Calculate the position of the one after that we want to remove */
		void *src_addr = (void*)(array + ((remove_index + 1) * elem_size));

		/* Calculate the destination */
		void *dest_addr = (void*)(array + (remove_index * elem_size));

		/* Shift the memory */
        memmove(dest_addr, src_addr, ((num_elems - 1) - remove_index) *
                elem_size);
        /* Clear that which was at the end */
		memset(array + (elem_size * (num_elems - 1)), 0, elem_size);
	}
}

struct allocated_volume {
    lsmVolumePtr v;
    lsmPoolPtr p;
};

struct allocated_fs {
    lsmFsPtr fs;
    lsmPoolPtr p;
    GHashTable *ss;
    GHashTable *exports;
};

struct allocated_ag {
    lsmAccessGroupPtr ag;
    lsmInitiatorType ag_type;
};

struct plugin_data {
    uint32_t tmo;
    uint32_t num_systems;
    lsmSystemPtr system[MAX_SYSTEMS];

    uint32_t num_pools;
    lsmPoolPtr pool[MAX_POOLS];

    uint32_t num_volumes;
    struct allocated_volume volume[MAX_VOLUMES];

    GHashTable *access_groups;
    GHashTable *group_grant;
    GHashTable *fs;

    GHashTable *jobs;
};

struct allocated_job {
    int polls;
    lsmDataType type;
    void *return_data;
};

struct allocated_job *alloc_allocated_job( lsmDataType type, void *return_data )
{
    struct allocated_job *rc = malloc( sizeof(struct allocated_job) );
    if( rc ) {
        rc->polls = 0;
        rc->type = type;
        rc->return_data = return_data;
    }
    return rc;
}

void free_allocated_job(void *j)
{
    struct allocated_job *job = j;

    if( job &&  job->return_data ) {
        switch( job->type ) {
        case(LSM_DATA_TYPE_BLOCK_RANGE):
            lsmBlockRangeRecordFree((lsmBlockRangePtr)job->return_data);
            break;
        case(LSM_DATA_TYPE_FS):
            lsmFsRecordFree((lsmFsPtr)job->return_data);
            break;
        case(LSM_DATA_TYPE_INITIATOR):
            lsmInitiatorRecordFree((lsmInitiatorPtr)job->return_data);
            break;
        case(LSM_DATA_TYPE_NFS_EXPORT):
            lsmNfsExportRecordFree((lsmNfsExportPtr)job->return_data);
            break;
        case(LSM_DATA_TYPE_POOL):
            lsmPoolRecordFree((lsmPoolPtr)job->return_data);
            break;
        case(LSM_DATA_TYPE_SS):
            lsmSsRecordFree((lsmSsPtr)job->return_data);
            break;
        case(LSM_DATA_TYPE_STRING_LIST):
            lsmStringListFree((lsmStringListPtr)job->return_data);
            break;
        case(LSM_DATA_TYPE_SYSTEM):
            lsmSystemRecordFree((lsmSystemPtr)job->return_data);
            break;
        case(LSM_DATA_TYPE_VOLUME):
            lsmVolumeRecordFree((lsmVolumePtr)job->return_data);
            break;
        default:
            break;
        }
        job->return_data = NULL;
    }
    free(job);
}

struct allocated_ag *alloc_allocated_ag( lsmAccessGroupPtr ag,
                                            lsmInitiatorType i)
{
    struct allocated_ag *aag =
                    (struct allocated_ag *)malloc(sizeof(struct allocated_ag));
    if( aag ) {
        aag->ag = ag;
        aag->ag_type = i;
    }
    return aag;
}

void free_allocated_ag(void *v)
{
    if( v ) {
        struct allocated_ag *aag = (struct allocated_ag *)v;
        lsmAccessGroupRecordFree(aag->ag);
        free(aag);
    }
}

void free_fs_record(struct allocated_fs *fs) {
    if( fs ) {
        g_hash_table_destroy(fs->ss);
        g_hash_table_destroy(fs->exports);
        lsmFsRecordFree(fs->fs);
        fs->p = NULL;
        free(fs);
    }
}

static void free_ss(void *s)
{
    lsmSsRecordFree((lsmSsPtr)s);
}

static void free_export(void *exp)
{
    lsmNfsExportRecordFree((lsmNfsExportPtr)exp);
}

static struct allocated_fs *alloc_fs_record() {
    struct allocated_fs *rc = (struct allocated_fs *)
                                malloc(sizeof(struct allocated_fs));
    if( rc ) {
        rc->fs = NULL;
        rc->p = NULL;
        rc->ss = g_hash_table_new_full(g_str_hash, g_str_equal, free, free_ss);
        rc->exports = g_hash_table_new_full(g_str_hash, g_str_equal, free,
                                            free_export);

        if( !rc->ss || !rc->exports ) {
            if( rc->ss ) {
                g_hash_table_destroy(rc->ss);
            }

            if( rc->exports ) {
                g_hash_table_destroy(rc->exports);
            }

            free(rc);
            rc = NULL;
        }
    }
    return rc;
}

static int create_job(struct plugin_data *pd, char **job, lsmDataType t,
                        void *new_value, void **returned_value)
{
    static int job_num = 0;
    int rc = LSM_ERR_JOB_STARTED;
    char job_id[64];
    char *key = NULL;

    /* Make this random */
    if( 0 ) {
        if( returned_value ) {
            *returned_value = new_value;
        }
        *job = NULL;
        rc = LSM_ERR_OK;
    } else {
        snprintf(job_id, sizeof(job_id), "JOB_%d", job_num);
        job_num += 1;

        if( returned_value ) {
            *returned_value = NULL;
        }

        *job = strdup(job_id);
        key = strdup(job_id);

        struct allocated_job *value = alloc_allocated_job(t, new_value);
        if( *job && key && value ) {
            g_hash_table_insert(pd->jobs, key, value);
        } else {
            free(*job);
            *job = NULL;
            free(key);
            key = NULL;
            free_allocated_job(value);
            value = NULL;
            rc = LSM_ERR_NO_MEMORY;
        }
    }
    return rc;
}

static int tmo_set(lsmPluginPtr c, uint32_t timeout, lsmFlag_t flags )
{
    struct plugin_data *pd = (struct plugin_data*)lsmGetPrivateData(c);
    pd->tmo = timeout;
    return LSM_ERR_OK;
}

static int tmo_get(lsmPluginPtr c, uint32_t *timeout, lsmFlag_t flags)
{
    struct plugin_data *pd = (struct plugin_data*)lsmGetPrivateData(c);
    *timeout = pd->tmo;
    return LSM_ERR_OK;
}

static int cap(lsmPluginPtr c, lsmSystemPtr system,
                lsmStorageCapabilitiesPtr *cap, lsmFlag_t flags)
{
    int rc = LSM_ERR_NO_MEMORY;
    *cap = lsmCapabilityRecordAlloc(NULL);

    if( *cap ) {
        rc = lsmCapabilitySetN(*cap, LSM_CAPABILITY_SUPPORTED, 48,
            LSM_CAP_BLOCK_SUPPORT,
            LSM_CAP_FS_SUPPORT,
            LSM_CAP_INITIATORS,
            LSM_CAP_VOLUMES,
            LSM_CAP_VOLUME_CREATE,
            LSM_CAP_VOLUME_RESIZE,
            LSM_CAP_VOLUME_REPLICATE,
            LSM_CAP_VOLUME_REPLICATE_CLONE,
            LSM_CAP_VOLUME_REPLICATE_COPY,
            LSM_CAP_VOLUME_REPLICATE_MIRROR_ASYNC,
            LSM_CAP_VOLUME_REPLICATE_MIRROR_SYNC,
            LSM_CAP_VOLUME_COPY_RANGE_BLOCK_SIZE,
            LSM_CAP_VOLUME_COPY_RANGE,
            LSM_CAP_VOLUME_COPY_RANGE_CLONE,
            LSM_CAP_VOLUME_COPY_RANGE_COPY,
            LSM_CAP_VOLUME_DELETE,
            LSM_CAP_VOLUME_ONLINE,
            LSM_CAP_VOLUME_OFFLINE,
            LSM_CAP_ACCESS_GROUP_GRANT,
            LSM_CAP_ACCESS_GROUP_REVOKE,
            LSM_CAP_ACCESS_GROUP_LIST,
            LSM_CAP_ACCESS_GROUP_CREATE,
            LSM_CAP_ACCESS_GROUP_DELETE,
            LSM_CAP_ACCESS_GROUP_ADD_INITIATOR,
            LSM_CAP_ACCESS_GROUP_DEL_INITIATOR,
            LSM_CAP_VOLUMES_ACCESSIBLE_BY_ACCESS_GROUP,
            LSM_CAP_ACCESS_GROUPS_GRANTED_TO_VOLUME,
            LSM_CAP_VOLUME_CHILD_DEPENDENCY,
            LSM_CAP_VOLUME_CHILD_DEPENDENCY_RM,
            LSM_CAP_FS,
            LSM_CAP_FS_DELETE,
            LSM_CAP_FS_RESIZE,
            LSM_CAP_FS_CREATE,
            LSM_CAP_FS_CLONE,
            LSM_CAP_FILE_CLONE,
            LSM_CAP_FS_SNAPSHOTS,
            LSM_CAP_FS_SNAPSHOT_CREATE,
            LSM_CAP_FS_SNAPSHOT_CREATE_SPECIFIC_FILES,
            LSM_CAP_FS_SNAPSHOT_DELETE,
            LSM_CAP_FS_SNAPSHOT_REVERT,
            LSM_CAP_FS_SNAPSHOT_REVERT_SPECIFIC_FILES,
            LSM_CAP_FS_CHILD_DEPENDENCY,
            LSM_CAP_FS_CHILD_DEPENDENCY_RM,
            LSM_CAP_FS_CHILD_DEPENDENCY_RM_SPECIFIC_FILES,
            LSM_CAP_EXPORT_AUTH,
            LSM_CAP_EXPORTS,
            LSM_CAP_EXPORT_FS,
            LSM_CAP_EXPORT_REMOVE
            );

        if( LSM_ERR_OK != rc ) {
            lsmCapabilityRecordFree(*cap);
            *cap = NULL;
        }
    }
    return rc;
}

static int jobStatus(lsmPluginPtr c, const char *job_id,
                        lsmJobStatus *status, uint8_t *percentComplete,
                        lsmDataType *t,
                        void **value, lsmFlag_t flags)
{
    int rc = LSM_ERR_OK;
    struct plugin_data *pd = (struct plugin_data*)lsmGetPrivateData(c);

    struct allocated_job *val = (struct allocated_job*)
                                    g_hash_table_lookup(pd->jobs,job_id);
    if( val ) {
        *status = LSM_JOB_INPROGRESS;

        val->polls += 34;

        if( (val->polls) >= 100 ) {
            *t = val->type;
            *value = lsmDataTypeCopy(val->type, val->return_data);
            *status = LSM_JOB_COMPLETE;
            *percentComplete = 100;
        } else {
            *percentComplete = val->polls;
        }

    } else {
        rc = LSM_ERR_NOT_FOUND_JOB;
    }

    return rc;
}

static int list_pools(lsmPluginPtr c, lsmPoolPtr **poolArray,
                                        uint32_t *count, lsmFlag_t flags)
{
    int rc = LSM_ERR_OK;
    struct plugin_data *pd = (struct plugin_data*)lsmGetPrivateData(c);
    *count = pd->num_pools;

    *poolArray = lsmPoolRecordAllocArray( pd->num_pools );
    if( *poolArray ) {
        uint32_t i = 0;
        for( i = 0; i < pd->num_pools; ++i ) {
            (*poolArray)[i] = lsmPoolRecordCopy(pd->pool[i]);
            if( !(*poolArray)[i] ) {
                rc = LSM_ERR_NO_MEMORY;
                lsmPoolRecordFreeArray(*poolArray, i);
                *poolArray = NULL;
                *count = 0;
                break;
            }
        }
    } else {
        rc = LSM_ERR_NO_MEMORY;
    }

    return rc;
}

static int list_systems(lsmPluginPtr c, lsmSystemPtr **systems,
                                        uint32_t *systemCount, lsmFlag_t flags)
{
    struct plugin_data *pd = (struct plugin_data*)lsmGetPrivateData(c);

    *systemCount = pd->num_systems;
    *systems = lsmSystemRecordAllocArray(MAX_SYSTEMS);

    if( *systems ) {
        (*systems)[0] = lsmSystemRecordCopy(pd->system[0]);

        if( (*systems)[0] ) {
            return LSM_ERR_OK;
        } else {
            lsmSystemRecordFreeArray(*systems, pd->num_systems);
        }
    }
    return LSM_ERR_NO_MEMORY;
}

static int jobFree(lsmPluginPtr c, char *job_id, lsmFlag_t flags)
{
    int rc = LSM_ERR_OK;
    struct plugin_data *pd = (struct plugin_data*)lsmGetPrivateData(c);

    if( !g_hash_table_remove(pd->jobs, job_id) ) {
        rc = LSM_ERR_NOT_FOUND_JOB;
    }
    return rc;
}

static struct lsmMgmtOps mgmOps = {
    tmo_set,
    tmo_get,
    cap,
    jobStatus,
    jobFree,
    list_pools,
    list_systems,
};

void freeInitiator(void *i) {
    if( i ) {
        lsmInitiatorRecordFree((lsmInitiatorPtr)i);
    }
}

static int list_initiators(lsmPluginPtr c, lsmInitiatorPtr **initArray,
                                        uint32_t *count, lsmFlag_t flags)
{
    int rc = LSM_ERR_OK;
    struct plugin_data *pd = (struct plugin_data*)lsmGetPrivateData(c);

    GHashTable *tmp_inits = g_hash_table_new_full(g_str_hash, g_str_equal, free,
                                                    freeInitiator);

    *count = 0;
    *initArray = NULL;

    if( tmp_inits ) {
        char *k = NULL;
        struct allocated_ag *v = NULL;
        GHashTableIter iter;

        g_hash_table_iter_init (&iter, pd->access_groups);
        while (g_hash_table_iter_next (&iter, (gpointer) &k, (gpointer)&v) ) {
            uint32_t i = 0;

            lsmStringListPtr inits = lsmAccessGroupInitiatorIdGet(v->ag);

            for(i = 0; i < lsmStringListSize(inits); ++i ) {
                char *init_key = strdup(lsmStringListGetElem(inits,i));
                lsmInitiatorPtr init_val = lsmInitiatorRecordAlloc(v->ag_type,
                                                                  init_key, "");

                if( init_key && init_val ) {
                    g_hash_table_insert(tmp_inits, init_key, init_val);
                } else {
                    g_hash_table_destroy(tmp_inits);
                    rc = LSM_ERR_NO_MEMORY;
                }
            }
        }

        if( LSM_ERR_OK == rc ) {
            *count = g_hash_table_size(tmp_inits);

            if( *count ) {
                *initArray = lsmInitiatorRecordAllocArray(*count);
                if( *initArray ) {
                    int i = 0;
                    char *ikey = NULL;
                    lsmInitiatorPtr ival = NULL;

                    g_hash_table_iter_init (&iter, tmp_inits);
                    while (g_hash_table_iter_next (&iter, (gpointer) &ikey,
                                                            (gpointer)&ival) ) {
                        (*initArray)[i] = lsmInitiatorRecordCopy(ival);
                        if( !(*initArray)[i] ) {
                            rc = LSM_ERR_NO_MEMORY;
                            lsmInitiatorRecordFreeArray(*initArray, i);
                            break;
                        }
                        i += 1;
                    }
                }
            }
        }

        g_hash_table_destroy(tmp_inits);
    }
    return rc;
}

static int list_volumes(lsmPluginPtr c, lsmVolumePtr **vols,
                                        uint32_t *count, lsmFlag_t flags)
{
    int rc = LSM_ERR_OK;
    struct plugin_data *pd = (struct plugin_data*)lsmGetPrivateData(c);
    *count = pd->num_volumes;

    if( *count ) {
        *vols = lsmVolumeRecordAllocArray( pd->num_volumes );
        if( *vols ) {
            uint32_t i = 0;
            for( i = 0; i < pd->num_volumes; ++i ) {
                (*vols)[i] = lsmVolumeRecordCopy(pd->volume[i].v);
                if( !(*vols)[i] ) {
                    rc = LSM_ERR_NO_MEMORY;
                    lsmVolumeRecordFreeArray(*vols, i);
                    *vols = NULL;
                    *count = 0;
                    break;
                }
            }
        } else {
            rc = LSM_ERR_NO_MEMORY;
        }
    }

    return rc;
}

static uint64_t pool_allocate(lsmPoolPtr p, uint64_t size)
{
    uint64_t rounded_size = 0;
    int rc = LSM_ERR_OK;
    uint64_t free_space = lsmPoolFreeSpaceGet(p);

    rounded_size = (size/BS) * BS;

    if( free_space >= rounded_size ) {
        free_space -= rounded_size;
        lsmPoolFreeSpaceSet(p, free_space);
    } else {
        rc = 0;
    }
    return rounded_size;
}

void pool_deallocate(lsmPoolPtr p, uint64_t size)
{
    uint64_t free_space = lsmPoolFreeSpaceGet(p);

    free_space += size;
    lsmPoolFreeSpaceSet(p, free_space);
}

static int find_pool(struct plugin_data *pd, const char* pool_id)
{
    if( pd ) {
        int i;
        for( i = 0; i < pd->num_pools; ++i ) {
            if( strcmp(lsmPoolIdGet(pd->pool[i]), pool_id) == 0) {
                return i;
            }
        }
    }
    return -1;
}

static int find_volume_name(struct plugin_data *pd, const char *name)
{
    if( pd ) {
        int i;
        for( i = 0; i < pd->num_volumes; ++i ) {
            if( strcmp(lsmVolumeNameGet(pd->volume[i].v), name) == 0) {
                return i;
            }
        }
    }
    return -1;
}

static int volume_create(lsmPluginPtr c, lsmPoolPtr pool,
                        const char *volumeName, uint64_t size,
                        lsmProvisionType provisioning, lsmVolumePtr *newVolume,
                        char **job, lsmFlag_t flags)
{
    int rc = LSM_ERR_OK;

    struct plugin_data *pd = (struct plugin_data*)lsmGetPrivateData(c);
    int pool_index = find_pool(pd, lsmPoolIdGet(pool));

    if( pool_index >= 0 ) {
        if( -1 == find_volume_name(pd, volumeName) ) {
            if ( pd->num_volumes < MAX_VOLUMES ) {
                lsmPoolPtr p = pd->pool[pool_index];
                uint64_t allocated_size = pool_allocate(p, size);
                if( allocated_size ) {
                    char *id = md5(volumeName);

                    lsmVolumePtr v = lsmVolumeRecordAlloc(id, volumeName,
                                       "VPD", BS, allocated_size/BS, 0, sys_id);

                    if( v ) {
                        pd->volume[pd->num_volumes].v = lsmVolumeRecordCopy(v);
                        pd->volume[pd->num_volumes].p = p;
                        pd->num_volumes +=1;

                        rc = create_job(pd, job, LSM_DATA_TYPE_VOLUME, v,
                                            (void**)newVolume);

                    } else {
                        rc = lsmLogErrorBasic(c, LSM_ERR_NO_MEMORY,
                                                "Check for leaks");
                    }

                } else {
                    rc = lsmLogErrorBasic(c, LSM_ERR_SIZE_INSUFFICIENT_SPACE,
                                                "Insufficient space in pool");
                }
            } else {
                rc = lsmLogErrorBasic(c, LSM_ERR_SIZE_LIMIT_REACHED,
                                            "Number of volumes limit reached");
            }
        } else {
            rc = lsmLogErrorBasic(c, LSM_ERR_EXISTS_NAME, "Existing volume "
                                                            "with name");
        }
    } else {
        rc = lsmLogErrorBasic(c, LSM_ERR_NOT_FOUND_POOL, "Pool not found!");
    }
    return rc;
}

static int volume_replicate(lsmPluginPtr c, lsmPoolPtr pool,
                        lsmReplicationType repType, lsmVolumePtr volumeSrc,
                        const char *name, lsmVolumePtr *newReplicant,
                        char **job, lsmFlag_t flags)
{
    int rc = LSM_ERR_OK;
    int pi;
    int vi;
    struct plugin_data *pd = (struct plugin_data*)lsmGetPrivateData(c);

    pi = find_pool(pd, lsmPoolIdGet(pool));
    vi = find_volume_name(pd, lsmVolumeNameGet(volumeSrc));

    if( pi > -1 && vi > -1 ) {
        rc = volume_create(c, pool, name,
                                lsmVolumeNumberOfBlocks(volumeSrc)*BS,
                                LSM_PROVISION_DEFAULT, newReplicant, job, flags);


    } else {
        if ( -1 == vi ) {
            rc = lsmLogErrorBasic(c, LSM_ERR_NOT_FOUND_VOLUME,
                                    "Volume not found!");
        }

        if( -1 == pi ) {
            rc = lsmLogErrorBasic(c, LSM_ERR_NOT_FOUND_POOL,
                                    "Pool not found!");
        }
    }
    return rc;
}

static int volume_replicate_range_bs(lsmPluginPtr c, uint32_t *bs,
                                    lsmFlag_t flags)
{
    *bs = BS;
    return LSM_ERR_OK;
}

static int volume_replicate_range(lsmPluginPtr c,
                                    lsmReplicationType repType,
                                    lsmVolumePtr source,
                                    lsmVolumePtr dest,
                                    lsmBlockRangePtr *ranges,
                                    uint32_t num_ranges, char **job,
                                    lsmFlag_t flags)
{
    int rc = LSM_ERR_OK;
    struct plugin_data *pd = (struct plugin_data*)lsmGetPrivateData(c);

    int src_v = find_volume_name(pd, lsmVolumeNameGet(source));
    int dest_v = find_volume_name(pd, lsmVolumeNameGet(dest));

    if( -1 == src_v || -1 == dest_v ) {
        rc = lsmLogErrorBasic(c, LSM_ERR_NOT_FOUND_VOLUME,
                                    "Src or dest volumes not found!");
    } else {
        rc = create_job(pd, job, LSM_DATA_TYPE_NONE, NULL, NULL);
    }

    return rc;
}

static int volume_resize(lsmPluginPtr c, lsmVolumePtr volume,
                                uint64_t newSize, lsmVolumePtr *resizedVolume,
                                char **job, lsmFlag_t flags)
{
    int rc = LSM_ERR_OK;
    int vi;
    struct plugin_data *pd = (struct plugin_data*)lsmGetPrivateData(c);

    vi = find_volume_name(pd, lsmVolumeNameGet(volume));

    if( -1 != vi ) {
        lsmVolumePtr v = pd->volume[vi].v;
        lsmPoolPtr p = pd->volume[vi].p;
        uint64_t curr_size = lsmVolumeNumberOfBlocks(v) * BS;

        pool_deallocate(p, curr_size);
        uint64_t resized_size = pool_allocate(p, newSize);
        if( resized_size ) {
            lsmVolumePtr vp = lsmVolumeRecordAlloc(lsmVolumeIdGet(v),
                                                    lsmVolumeNameGet(v),
                                                    lsmVolumeVpd83Get(v),
                                                    lsmVolumeBlockSizeGet(v),
                                                    resized_size/BS, 0, sys_id);
            if( vp ) {
                pd->volume[vi].v = vp;
                lsmVolumeRecordFree(v);
                rc = create_job(pd, job, LSM_DATA_TYPE_VOLUME,
                                    lsmVolumeRecordCopy(vp),
                                    (void**)resizedVolume);
            } else {
                pool_deallocate(p, resized_size);
                pool_allocate(p, curr_size);
                rc = lsmLogErrorBasic(c, LSM_ERR_NO_MAPPING, "ENOMEM");
            }

        } else {
            /*Could not accommodate re-sized, go back */
            pool_allocate(p, curr_size);
            rc = lsmLogErrorBasic(c, LSM_ERR_SIZE_INSUFFICIENT_SPACE,
                                                "Insufficient space in pool");
        }

    } else {
        rc = lsmLogErrorBasic(c, LSM_ERR_NOT_FOUND_VOLUME,
                                    "volume not found!");
    }
    return rc;
}

static int volume_delete(lsmPluginPtr c, lsmVolumePtr volume,
                                    char **job, lsmFlag_t flags)
{
    int rc = LSM_ERR_OK;
    GHashTableIter iter;
    char *k = NULL;
    GHashTable *v = NULL;
    const char *volume_id = lsmVolumeIdGet(volume);
    struct plugin_data *pd = (struct plugin_data*)lsmGetPrivateData(c);
    int vi = find_volume_name(pd, lsmVolumeNameGet(volume));
    if( -1 != vi ) {
        lsmVolumePtr vp = pd->volume[vi].v;
        pool_deallocate(pd->volume[vi].p, lsmVolumeNumberOfBlocks(vp)*BS);

        lsmVolumeRecordFree(vp);
        remove_item(pd->volume, vi, pd->num_volumes,
                        sizeof(struct allocated_volume));
        pd->num_volumes -= 1;

        g_hash_table_iter_init (&iter, pd->group_grant);
        while( g_hash_table_iter_next( &iter, (gpointer)&k, (gpointer)&v) ) {
            if( g_hash_table_lookup(v, volume_id) ) {
                g_hash_table_remove(v, volume_id );
            }
        }

        rc = create_job(pd, job, LSM_DATA_TYPE_NONE, NULL, NULL);

    } else {
        rc = lsmLogErrorBasic(c, LSM_ERR_NOT_FOUND_VOLUME,
                                    "volume not found!");
    }
    return rc;
}

static int volume_online_offline(lsmPluginPtr c, lsmVolumePtr v,
                                    lsmFlag_t flags)
{
    int rc = LSM_ERR_OK;
    struct plugin_data *pd = (struct plugin_data*)lsmGetPrivateData(c);

    int vi = find_volume_name(pd, lsmVolumeNameGet(v));
    if( -1 == vi ) {
        rc = lsmLogErrorBasic(c, LSM_ERR_NOT_FOUND_VOLUME,
                                    "volume not found!");
    }
    return rc;
}

static int access_group_list(lsmPluginPtr c,
                                lsmAccessGroupPtr **groups,
                                uint32_t *groupCount, lsmFlag_t flags)
{
    int rc = LSM_ERR_OK;
    struct plugin_data *pd = (struct plugin_data*)lsmGetPrivateData(c);

    *groupCount = g_hash_table_size(pd->access_groups);

    if( *groupCount ) {
        *groups = lsmAccessGroupRecordAllocArray(*groupCount);
        if( *groups ) {
            int i = 0;
            char *key = NULL;
            struct allocated_ag *val = NULL;
            GHashTableIter iter;

            g_hash_table_iter_init (&iter, pd->access_groups);

            while (g_hash_table_iter_next (&iter, (gpointer) &key,
                                                    (gpointer)&val) ) {
                (*groups)[i] = lsmAccessGroupRecordCopy(val->ag);
                if( !(*groups)[i] ) {
                    rc = LSM_ERR_NO_MEMORY;
                    lsmAccessGroupRecordFreeArray(*groups, i);
                    *groupCount = 0;
                    groups = NULL;
                    break;
                }
                ++i;
            }
        } else {
            rc = LSM_ERR_NO_MEMORY;
        }
    }
    return rc;
}

static int access_group_create(lsmPluginPtr c,
                                const char *name,
                                const char *initiator_id,
                                lsmInitiatorType id_type,
                                const char *system_id,
                                lsmAccessGroupPtr *access_group,
                                lsmFlag_t flags)
{
    int rc = LSM_ERR_OK;
    lsmAccessGroupPtr ag = NULL;
    struct allocated_ag *aag = NULL;
    struct plugin_data *pd = (struct plugin_data*)lsmGetPrivateData(c);
    char *id = strdup(md5(name));

    struct allocated_ag *find = (struct allocated_ag *)
                        g_hash_table_lookup(pd->access_groups, id);

    if( !find ) {
        lsmStringListPtr initiators = lsmStringListAlloc(1);
        if( initiators && id &&
            (LSM_ERR_OK == lsmStringListSetElem(initiators, 0, initiator_id))) {
            ag = lsmAccessGroupRecordAlloc(id, name, initiators,
                                                        system_id);
            aag = alloc_allocated_ag(ag, id_type);
            if( ag && aag ) {
                g_hash_table_insert(pd->access_groups, (gpointer)id,
                                        (gpointer)aag);
                *access_group = lsmAccessGroupRecordCopy(ag);
            } else {
                free_allocated_ag(aag);
                lsmAccessGroupRecordFree(ag);
                rc = lsmLogErrorBasic(c, LSM_ERR_NO_MEMORY, "ENOMEM");
            }
        } else {
            rc = lsmLogErrorBasic(c, LSM_ERR_NO_MEMORY,
                                    "ENOMEM");
        }

        /* Initiators is copied when allocating a group record */
        lsmStringListFree(initiators);

    } else {
        rc = lsmLogErrorBasic(c, LSM_ERR_EXISTS_ACCESS_GROUP,
                                    "access group with same id found");
    }

    /*
     *  If we were not successful free memory for id string, id is on the heap
     * because it is passed to the hash table.
     */
    if( LSM_ERR_OK != rc ) {
        free(id);
    }

    return rc;
}

static int access_group_delete( lsmPluginPtr c,
                                lsmAccessGroupPtr group,
                                char **job, lsmFlag_t flags)
{
    int rc = LSM_ERR_OK;

    struct plugin_data *pd = (struct plugin_data*)lsmGetPrivateData(c);
    const char *id = lsmAccessGroupIdGet(group);

    gboolean r = g_hash_table_remove(pd->access_groups, (gpointer)id);

    if( !r ) {
        rc = lsmLogErrorBasic(c, LSM_ERR_NOT_FOUND_ACCESS_GROUP,
                                    "access group not found");
    }
    return rc;
}

static int access_group_add_initiator(  lsmPluginPtr c,
                                        lsmAccessGroupPtr group,
                                        const char *initiator_id,
                                        lsmInitiatorType id_type, char **job,
                                        lsmFlag_t flags)
{
    int rc = LSM_ERR_OK;
    struct plugin_data *pd = (struct plugin_data*)lsmGetPrivateData(c);

    struct allocated_ag *find = (struct allocated_ag *)
                                g_hash_table_lookup(pd->access_groups,
                                lsmAccessGroupIdGet(group));

    if( find ) {
        lsmStringList *inits = lsmAccessGroupInitiatorIdGet(find->ag);
        rc = lsmStringListAppend(inits, initiator_id);
        if( LSM_ERR_OK == rc ) {
            rc = create_job(pd, job, LSM_DATA_TYPE_NONE, NULL, NULL);
        }
    } else {
        rc = lsmLogErrorBasic(c, LSM_ERR_NOT_FOUND_ACCESS_GROUP,
                                    "access group not found");
    }
    return rc;
}

static int access_group_del_initiator(  lsmPluginPtr c,
                                        lsmAccessGroupPtr group,
                                        const char *init,
                                        char **job, lsmFlag_t flags)
{
    int rc = LSM_ERR_INITIATOR_NOT_IN_ACCESS_GROUP;
    struct plugin_data *pd = (struct plugin_data*)lsmGetPrivateData(c);

    struct allocated_ag *find = (struct allocated_ag *)
                                g_hash_table_lookup(pd->access_groups,
                                lsmAccessGroupIdGet(group));

    if( find ) {
        uint32_t i;
        lsmStringList *inits = lsmAccessGroupInitiatorIdGet(find->ag);

        for(i = 0; i < lsmStringListSize(inits); ++i) {
            if( strcmp(init, lsmStringListGetElem(inits, i)) == 0 ) {
                lsmStringListRemove(inits, i);
                rc = LSM_ERR_OK;
                break;
            }
        }

        if( LSM_ERR_OK == rc ) {
            rc = create_job(pd, job, LSM_DATA_TYPE_NONE, NULL, NULL);
        }
    } else {
        rc = lsmLogErrorBasic(c, LSM_ERR_NOT_FOUND_ACCESS_GROUP,
                                    "access group not found");
    }
    return rc;
}

static int access_group_grant(lsmPluginPtr c,
                                lsmAccessGroupPtr group,
                                lsmVolumePtr volume,
                                lsmAccessType access, char **job,
                                lsmFlag_t flags)
{
    int rc = LSM_ERR_OK;
    struct plugin_data *pd = (struct plugin_data*)lsmGetPrivateData(c);

    struct allocated_ag *find = (struct allocated_ag *)
                                g_hash_table_lookup(pd->access_groups,
                                lsmAccessGroupIdGet(group));

    int vi = find_volume_name(pd, lsmVolumeNameGet(volume));

    if( find && (-1 != vi) ) {
        GHashTable *grants = g_hash_table_lookup(pd->group_grant,
                                    lsmAccessGroupIdGet(find->ag));
        if( !grants ) {
            /* We don't have any mappings for this access group*/
            GHashTable *grant = g_hash_table_new_full(g_str_hash, g_str_equal,
                                                        free, free);
            char *key = strdup(lsmAccessGroupIdGet(find->ag));
            char *vol_id = strdup(lsmVolumeIdGet(volume));
            lsmAccessType *val = (lsmAccessType*) malloc(sizeof(lsmAccessType));

            if( grant && key && val && vol_id ) {
                *val = access;

                /* Create the association for volume id and access value */
                g_hash_table_insert(grant, vol_id, val);

                /* Create the association for access groups */
                g_hash_table_insert(pd->group_grant, key, grant);

                rc = create_job(pd, job, LSM_DATA_TYPE_NONE, NULL, NULL);

            } else {
                rc = LSM_ERR_NO_MEMORY;
                free(key);
                free(val);
                free(vol_id);
                if( grant ) {
                    g_hash_table_destroy(grant);
                    grant = NULL;
                }
            }

        } else {
            /* See if we have this volume in the access grants */
            char *vol_id = g_hash_table_lookup(grants, lsmVolumeIdGet(volume));
            if( !vol_id ) {
                vol_id = strdup(lsmVolumeIdGet(volume));
                lsmAccessType *val =
                                (lsmAccessType*) malloc(sizeof(lsmAccessType));
                if( vol_id && val ) {
                    g_hash_table_insert(grants, vol_id, val);
                } else {
                    rc = LSM_ERR_NO_MEMORY;
                    free(vol_id);
                    free(val);
                }

            } else {
                rc = LSM_ERR_IS_MAPPED;
            }
        }
    } else {
        if( -1 == vi ) {
            rc = lsmLogErrorBasic(c, LSM_ERR_NOT_FOUND_VOLUME,
                                        "volume not found");
        } else {
            rc = lsmLogErrorBasic(c, LSM_ERR_NOT_FOUND_ACCESS_GROUP,
                                    "access group not found");
        }
    }
    return rc;
}

static int access_group_revoke(lsmPluginPtr c,
                                lsmAccessGroupPtr group,
                                lsmVolumePtr volume, char **job,
                                lsmFlag_t flags)
{
    int rc = LSM_ERR_NO_MAPPING;
    struct plugin_data *pd = (struct plugin_data*)lsmGetPrivateData(c);

    struct allocated_ag *find = (struct allocated_ag *)
                                g_hash_table_lookup(pd->access_groups,
                                lsmAccessGroupIdGet(group));

    int vi = find_volume_name(pd, lsmVolumeNameGet(volume));

    if( find && (-1 != vi) ) {
        GHashTable *grants = g_hash_table_lookup(pd->group_grant,
                                    lsmAccessGroupIdGet(find->ag));

        if( (grants && g_hash_table_remove(grants, lsmVolumeIdGet(volume)))) {
            rc = create_job(pd, job, LSM_DATA_TYPE_NONE, NULL, NULL);
        }

    } else {
       if( -1 == vi ) {
            rc = lsmLogErrorBasic(c, LSM_ERR_NOT_FOUND_VOLUME,
                                        "volume not found");
        } else {
            rc = lsmLogErrorBasic(c, LSM_ERR_NOT_FOUND_ACCESS_GROUP,
                                    "access group not found");
        }
    }
    return rc;
}

static lsmVolumePtr get_volume_by_id(struct plugin_data *pd, const char *id)
{
    int i = 0;
    for( i = 0; i < pd->num_volumes; ++i ) {
        if( 0 == strcmp(id, lsmVolumeIdGet(pd->volume[i].v))) {
            return pd->volume[i].v;
        }
    }
    return NULL;
}

static int vol_accessible_by_ag(lsmPluginPtr c,
                                lsmAccessGroupPtr group,
                                lsmVolumePtr **volumes,
                                uint32_t *count, lsmFlag_t flags)
{
    int rc = LSM_ERR_OK;
    struct plugin_data *pd = (struct plugin_data*)lsmGetPrivateData(c);

    struct allocated_ag *find = (struct allocated_ag *)
                                g_hash_table_lookup(pd->access_groups,
                                lsmAccessGroupIdGet(group));
    if( find ) {
        GHashTable *grants = g_hash_table_lookup(pd->group_grant,
                                    lsmAccessGroupIdGet(find->ag));
        *count = 0;

        if( grants && g_hash_table_size(grants) ) {
            *count = g_hash_table_size(grants);
            GList *keys = g_hash_table_get_keys(grants);
            *volumes = lsmVolumeRecordAllocArray(*count);

            if( keys && *volumes ) {
                GList *curr = NULL;
                int i = 0;

                for(    curr = g_list_first(keys);
                        curr != NULL;
                        curr = g_list_next(curr), ++i ) {

                    (*volumes)[i] = lsmVolumeRecordCopy(get_volume_by_id(pd,
                                                        (char *)curr->data));
                    if( !(*volumes)[i] ) {
                        rc = LSM_ERR_NO_MEMORY;
                        lsmVolumeRecordFreeArray(*volumes, i);
                        *volumes = NULL;
                        *count = 0;
                        break;
                    }
                }

                /* Free the keys */
                g_list_free(keys);
            } else {
                rc = LSM_ERR_NO_MEMORY;
            }
        }

    } else {
        rc = lsmLogErrorBasic(c, LSM_ERR_NOT_FOUND_ACCESS_GROUP,
                                    "access group not found");
    }
    return rc;
}

static lsmAccessGroupPtr access_group_by_id(struct plugin_data *pd,
                                            const char *key)
{
    struct allocated_ag *find = g_hash_table_lookup(pd->access_groups, key);
    if(find) {
        return find->ag;
    }
    return NULL;
}

static int ag_granted_to_volume( lsmPluginPtr c,
                                    lsmVolumePtr volume,
                                    lsmAccessGroupPtr **groups,
                                    uint32_t *count, lsmFlag_t flags)
{
    int rc = LSM_ERR_OK;
    GHashTableIter iter;
    char *k = NULL;
    GHashTable *v = NULL;
    struct plugin_data *pd = (struct plugin_data*)lsmGetPrivateData(c);
    const char* volume_id = lsmVolumeIdGet(volume);
    g_hash_table_iter_init (&iter, pd->group_grant);
    GSList *result = NULL;

    *count = 0;

    while( g_hash_table_iter_next( &iter, (gpointer)&k, (gpointer)&v) ) {
        if( g_hash_table_lookup(v, volume_id) ) {
            *count += 1;
            result = g_slist_prepend(result, access_group_by_id(pd, k));
        }
    }

    if( *count ) {
        int i = 0;
        *groups = lsmAccessGroupRecordAllocArray(*count);
        GSList *siter = NULL;

        if( *groups ) {
            for( siter = result; siter ; siter = g_slist_next(siter), i++) {
                (*groups)[i] = lsmAccessGroupRecordCopy(
                                                (lsmAccessGroupPtr)siter->data);

                if( !(*groups)[i] ) {
                    rc = LSM_ERR_NO_MEMORY;
                    lsmAccessGroupRecordFreeArray(*groups, i);
                    *groups = NULL;
                    *count = 0;
                    break;
                }
            }
        } else {
            rc = LSM_ERR_NO_MEMORY;
        }
    }

    if(result) {
        g_slist_free(result);
    }
    return rc;
}

int static volume_dependency(lsmPluginPtr c,
                                            lsmVolumePtr volume,
                                            uint8_t *yes, lsmFlag_t flags)
{
    struct plugin_data *pd = (struct plugin_data*)lsmGetPrivateData(c);
    int vi = find_volume_name(pd, lsmVolumeNameGet(volume));

    if( -1 != vi ) {
        *yes = 0;
        return LSM_ERR_OK;
    } else {
        return LSM_ERR_NOT_FOUND_VOLUME;
    }
}

int static volume_dependency_rm(lsmPluginPtr c,
                                            lsmVolumePtr volume,
                                            char **job, lsmFlag_t flags)
{
    struct plugin_data *pd = (struct plugin_data*)lsmGetPrivateData(c);
    int vi = find_volume_name(pd, lsmVolumeNameGet(volume));

    if( -1 != vi ) {
        return create_job(pd, job, LSM_DATA_TYPE_NONE, NULL, NULL);
    } else {
        return LSM_ERR_NOT_FOUND_VOLUME;
    }
}

static struct lsmSanOps sanOps = {
    list_initiators,
    list_volumes,
    volume_create,
    volume_replicate,
    volume_replicate_range_bs,
    volume_replicate_range,
    volume_resize,
    volume_delete,
    volume_online_offline,
    volume_online_offline,
    access_group_list,
    access_group_create,
    access_group_delete,
    access_group_add_initiator,
    access_group_del_initiator,
    access_group_grant,
    access_group_revoke,
    vol_accessible_by_ag,
    ag_granted_to_volume,
    volume_dependency,
    volume_dependency_rm
};


static int fs_list(lsmPluginPtr c, lsmFsPtr **fs, uint32_t *count,
                    lsmFlag_t flags)
{
    int rc = LSM_ERR_OK;
    struct plugin_data *pd = (struct plugin_data*)lsmGetPrivateData(c);
    *count = g_hash_table_size(pd->fs);

    if( *count ) {
        *fs = lsmFsRecordAllocArray( *count );
        if( *fs ) {
            uint32_t i = 0;
            char *k = NULL;
            struct allocated_fs *afs = NULL;
            GHashTableIter iter;
            g_hash_table_iter_init(&iter, pd->fs);
            while(g_hash_table_iter_next(&iter,(gpointer) &k,(gpointer)&afs)) {
                (*fs)[i] = lsmFsRecordCopy(afs->fs);
                if( !(*fs)[i] ) {
                    rc = LSM_ERR_NO_MEMORY;
                    lsmFsRecordFreeArray(*fs, i);
                    *count = 0;
                    *fs = NULL;
                    break;
                }
                ++i;
            }
        } else {
            rc = lsmLogErrorBasic(c, LSM_ERR_NO_MEMORY, "ENOMEM");
        }
    }
    return rc;
}

static int fs_create(lsmPluginPtr c, lsmPoolPtr pool, const char *name,
                uint64_t size_bytes, lsmFsPtr *fs, char **job, lsmFlag_t flags)
{
    int rc = LSM_ERR_OK;
    struct plugin_data *pd = (struct plugin_data*)lsmGetPrivateData(c);

    int pi = find_pool(pd, lsmPoolIdGet(pool));


    if( -1 != pi && !g_hash_table_lookup(pd->fs, md5(name)) ) {
        lsmPoolPtr p = pd->pool[pi];
        uint64_t allocated_size = pool_allocate(p, size_bytes);
        if( allocated_size ) {
            char *id = md5(name);
            char *key = strdup(id);
            lsmFsPtr new_fs = NULL;

            /* Make a copy to store and a copy to hand back to caller */
            lsmFsPtr tfs = lsmFsRecordAlloc(id, name, allocated_size,
                                allocated_size, lsmPoolIdGet(pool), sys_id);
            new_fs = lsmFsRecordCopy(tfs);

            /* Allocate the memory to keep the associations */
            struct allocated_fs *afs = alloc_fs_record();

            if( key && tfs && afs ) {
                afs->fs = tfs;
                afs->p = p;
                g_hash_table_insert(pd->fs, key, afs);

                rc = create_job(pd, job, LSM_DATA_TYPE_FS, new_fs, (void**)fs);
            } else {
                free(key);
                lsmFsRecordFree(new_fs);
                lsmFsRecordFree(tfs);
                free_fs_record(afs);

                *fs = NULL;
                rc = lsmLogErrorBasic(c, LSM_ERR_NO_MEMORY, "ENOMEM");
            }
        } else {
            rc = lsmLogErrorBasic(c, LSM_ERR_SIZE_INSUFFICIENT_SPACE,
                                                "Insufficient space in pool");
        }
    } else {
        if( -1 == pi ) {
            rc = lsmLogErrorBasic(c, LSM_ERR_NOT_FOUND_POOL, "Pool not found!");
        } else {
            rc = lsmLogErrorBasic(c, LSM_ERR_EXISTS_NAME,
                                        "File system with name exists");
        }
    }
    return rc;
}

static int fs_delete(lsmPluginPtr c, lsmFsPtr fs, char **job, lsmFlag_t flags)
{
    int rc = LSM_ERR_OK;
    struct plugin_data *pd = (struct plugin_data*)lsmGetPrivateData(c);

    if( !g_hash_table_remove(pd->fs, lsmFsIdGet(fs)) ) {
        rc = lsmLogErrorBasic(c, LSM_ERR_NOT_FOUND_FS, "FS not found!");
    } else {
        rc = create_job(pd, job, LSM_DATA_TYPE_NONE, NULL, NULL);
    }
    return rc;
}

static int fs_resize(lsmPluginPtr c, lsmFsPtr fs,
                                    uint64_t new_size_bytes, lsmFsPtr *rfs,
                                    char **job, lsmFlag_t flags)
{
    int rc = LSM_ERR_OK;
    struct plugin_data *pd = (struct plugin_data*)lsmGetPrivateData(c);
    struct allocated_fs *afs = g_hash_table_lookup(pd->fs, lsmFsIdGet(fs));

    *rfs = NULL;
    *job = NULL;

    if( afs ) {
        lsmPoolPtr p = afs->p;
        lsmFsPtr tfs = afs->fs;

        pool_deallocate(p, lsmFsTotalSpaceGet(tfs));
        uint64_t resized_size = pool_allocate(p, new_size_bytes);

        if( resized_size ) {

            lsmFsPtr resized = lsmFsRecordAlloc(lsmFsIdGet(tfs),
                                                lsmFsNameGet(tfs),
                                                new_size_bytes,
                                                new_size_bytes,
                                                lsmFsPoolIdGet(tfs),
                                                lsmFsSystemIdGet(tfs));
            lsmFsPtr returned_copy = lsmFsRecordCopy(resized);

            if( resized && returned_copy ) {
                lsmFsRecordFree(tfs);
                afs->fs = resized;

                rc = create_job(pd, job, LSM_DATA_TYPE_FS, returned_copy,
                                    (void**)rfs);

            } else {
                lsmFsRecordFree(resized);
                lsmFsRecordFree(returned_copy);
                *rfs = NULL;

                pool_deallocate(p, new_size_bytes);
                pool_allocate(p, lsmFsTotalSpaceGet(tfs));
                rc = lsmLogErrorBasic(c, LSM_ERR_NO_MEMORY,
                                    "ENOMEM");
            }
        } else {
            /*Could not accommodate re-sized, go back */
            pool_allocate(p, lsmFsTotalSpaceGet(tfs));
            rc = lsmLogErrorBasic(c, LSM_ERR_SIZE_INSUFFICIENT_SPACE,
                                                "Insufficient space in pool");
        }

    } else {
        rc = lsmLogErrorBasic(c, LSM_ERR_NOT_FOUND_FS,
                                    "file system not found!");
    }
    return rc;
}

static int fs_clone(lsmPluginPtr c, lsmFsPtr src_fs, const char *dest_fs_name,
                    lsmFsPtr *cloned_fs, lsmSsPtr optional_snapshot,
                    char **job, lsmFlag_t flags)
{
    int rc = LSM_ERR_OK;
    struct plugin_data *pd = (struct plugin_data*)lsmGetPrivateData(c);

    struct allocated_fs *find = g_hash_table_lookup(pd->fs, lsmFsIdGet(src_fs));

    if( find ) {
        rc = fs_create(c, find->p, dest_fs_name, lsmFsTotalSpaceGet(find->fs),
                        cloned_fs, job, flags);
    } else {
        rc = lsmLogErrorBasic(c, LSM_ERR_NOT_FOUND_FS, "Source fs not found");
    }

    return rc;
}

static int fs_file_clone(lsmPluginPtr c, lsmFsPtr fs,
                                    const char *src_file_name,
                                    const char *dest_file_name,
                                    lsmSsPtr snapshot, char **job,
                                    lsmFlag_t flags)
{
    int rc = LSM_ERR_OK;
    struct plugin_data *pd = (struct plugin_data*)lsmGetPrivateData(c);

    struct allocated_fs *find = (struct allocated_fs *)g_hash_table_lookup(
                                    pd->fs, lsmFsIdGet(fs));
    if( !find ) {
        rc = lsmLogErrorBasic(c, LSM_ERR_NOT_FOUND_FS, "fs not found");
    } else {
        rc = create_job(pd, job, LSM_DATA_TYPE_NONE, NULL, NULL);
    }
    return rc;
}

static int fs_child_dependency(lsmPluginPtr c, lsmFsPtr fs,
                                                lsmStringListPtr files,
                                                uint8_t *yes)
{
    int rc = LSM_ERR_OK;
    struct plugin_data *pd = (struct plugin_data*)lsmGetPrivateData(c);
    if( g_hash_table_lookup(pd->fs, lsmFsIdGet(fs))) {
        *yes = 0;
    } else {
        rc = lsmLogErrorBasic(c, LSM_ERR_NOT_FOUND_FS, "fs not found");
    }
    return rc;
}

static int fs_child_dependency_rm( lsmPluginPtr c, lsmFsPtr fs,
                                                lsmStringListPtr files,
                                                char **job, lsmFlag_t flags)
{
    int rc = LSM_ERR_OK;
    struct plugin_data *pd = (struct plugin_data*)lsmGetPrivateData(c);
    if( !g_hash_table_lookup(pd->fs, lsmFsIdGet(fs))) {
        rc = lsmLogErrorBasic(c, LSM_ERR_NOT_FOUND_FS, "fs not found");
    } else {
        rc = create_job(pd, job, LSM_DATA_TYPE_NONE, NULL, NULL);
    }
    return rc;
}

static int ss_list(lsmPluginPtr c, lsmFsPtr fs, lsmSsPtr **ss,
                                uint32_t *count, lsmFlag_t flags)
{
    int rc = LSM_ERR_OK;
    struct plugin_data *pd = (struct plugin_data*)lsmGetPrivateData(c);

    struct allocated_fs *find = (struct allocated_fs *)g_hash_table_lookup(
                                    pd->fs, lsmFsIdGet(fs));

    if( find ) {
        char *k = NULL;
        lsmSsPtr v = NULL;
        GHashTableIter iter;

        *ss = NULL;
        *count = g_hash_table_size(find->ss);

        if( *count ) {
            *ss = lsmSsRecordAllocArray(*count);
            if( *ss ) {
                int i = 0;
                g_hash_table_iter_init(&iter, find->ss);

                while(g_hash_table_iter_next(&iter,
                                            (gpointer) &k,(gpointer)&v)) {
                    (*ss)[i] = lsmSsRecordCopy(v);
                    if( !(*ss)[i] ) {
                        rc = lsmLogErrorBasic(c, LSM_ERR_NO_MEMORY, "ENOMEM");
                        lsmSsRecordFreeArray(*ss, i);
                        *ss = NULL;
                        *count = 0;
                        break;
                    }
                    ++i;
                }

            } else {
                rc = lsmLogErrorBasic(c, LSM_ERR_NO_MEMORY, "ENOMEM");
                *count = 0;
            }
        }
    } else {
        rc = lsmLogErrorBasic(c, LSM_ERR_NOT_FOUND_FS, "fs not found");
    }
    return rc;
}

static int ss_create(lsmPluginPtr c, lsmFsPtr fs,
                                    const char *name, lsmStringListPtr files,
                                    lsmSsPtr *snapshot, char **job,
                                    lsmFlag_t flags)
{
    int rc = LSM_ERR_NO_MEMORY;
    struct plugin_data *pd = (struct plugin_data*)lsmGetPrivateData(c);

    struct allocated_fs *find = (struct allocated_fs *)g_hash_table_lookup(
                                    pd->fs, lsmFsIdGet(fs));

    if( find ) {
        if( !g_hash_table_lookup(find->ss, md5(name)) ) {
            char *id = strdup(md5(name));
            if( id ) {
                lsmSsPtr ss = lsmSsRecordAlloc(id, name, time(NULL));
                lsmSsPtr new_shot = lsmSsRecordCopy(ss);
                if( ss && new_shot ) {
                    g_hash_table_insert(find->ss, (gpointer)id, (gpointer)ss);
                    rc = create_job(pd, job, LSM_DATA_TYPE_SS, new_shot,
                                        (void**)snapshot);
                } else {
                    lsmSsRecordFree(ss);
                    ss = NULL;
                    lsmSsRecordFree(new_shot);
                    *snapshot = NULL;
                    free(id);
                    id = NULL;
                }
            }
        } else {
            rc = lsmLogErrorBasic(c, LSM_ERR_EXISTS_NAME,
                                        "snapshot name exists");
        }
    } else {
        rc = lsmLogErrorBasic(c, LSM_ERR_NOT_FOUND_FS, "fs not found");
    }
    return rc;
}

static int ss_delete(lsmPluginPtr c, lsmFsPtr fs, lsmSsPtr ss,
                                    char **job, lsmFlag_t flags)
{
    int rc = LSM_ERR_OK;
    struct plugin_data *pd = (struct plugin_data*)lsmGetPrivateData(c);

    struct allocated_fs *find = (struct allocated_fs *)g_hash_table_lookup(
                                    pd->fs, lsmFsIdGet(fs));

    if( find ) {
        if( !g_hash_table_remove(find->ss, lsmSsIdGet(ss)) ) {
            rc = lsmLogErrorBasic(c, LSM_ERR_NOT_FOUND_SS,
                                    "snapshot not found");
        } else {
            rc = create_job(pd, job, LSM_DATA_TYPE_NONE, NULL, NULL);
        }
    } else {
        rc = lsmLogErrorBasic(c, LSM_ERR_NOT_FOUND_FS, "fs not found");
    }
    return rc;
}

static int ss_revert(lsmPluginPtr c, lsmFsPtr fs, lsmSsPtr ss,
                                    lsmStringListPtr files,
                                    lsmStringListPtr restore_files,
                                    int all_files, char **job, lsmFlag_t flags)
{
    int rc = LSM_ERR_OK;
    struct plugin_data *pd = (struct plugin_data*)lsmGetPrivateData(c);

    struct allocated_fs *find = (struct allocated_fs *)g_hash_table_lookup(
                                    pd->fs, lsmFsIdGet(fs));

    if( find ) {
        if(!g_hash_table_lookup(find->ss, lsmSsIdGet(ss))) {
            rc = lsmLogErrorBasic(c, LSM_ERR_NOT_FOUND_SS,
                                    "snapshot not found");
        } else {
            rc = create_job(pd, job, LSM_DATA_TYPE_NONE, NULL, NULL);
        }
    } else {
        rc = lsmLogErrorBasic(c, LSM_ERR_NOT_FOUND_FS, "fs not found");
    }
    return rc;
}

static struct lsmFsOps fsOps = {
    fs_list,
    fs_create,
    fs_delete,
    fs_resize,
    fs_clone,
    fs_file_clone,
    fs_child_dependency,
    fs_child_dependency_rm,
    ss_list,
    ss_create,
    ss_delete,
    ss_revert
};

static int nfs_auth_types(lsmPluginPtr c, lsmStringListPtr *types,
                            lsmFlag_t flags)
{
    int rc = LSM_ERR_OK;
    *types = lsmStringListAlloc(1);
    if( *types ) {
        lsmStringListSetElem(*types, 0, "standard");
    } else {
        rc = LSM_ERR_NO_MEMORY;
    }
    return rc;
}

static int nfs_export_list( lsmPluginPtr c, lsmNfsExportPtr **exports,
                                uint32_t *count, lsmFlag_t flags)
{
    int rc = LSM_ERR_OK;
    GHashTableIter fs_iter;
    GHashTableIter exports_iter;
    char *k = NULL;
    struct allocated_fs *v = NULL;
    GSList *result = NULL;
    struct plugin_data *pd = (struct plugin_data*)lsmGetPrivateData(c);

    g_hash_table_iter_init (&fs_iter, pd->fs);

    *count = 0;

    /* Walk through each of the file systems and their associated exports */
    while( g_hash_table_iter_next( &fs_iter, (gpointer)&k, (gpointer)&v) ) {
        char *exp_key = NULL;
        lsmNfsExportPtr *exp_val = NULL;

        g_hash_table_iter_init (&exports_iter, v->exports );
        while( g_hash_table_iter_next( &exports_iter,   (gpointer)&exp_key,
                                                        (gpointer)&exp_val) ) {
            result = g_slist_prepend(result, exp_val);
            *count += 1;
        }
    }

    if( *count ) {
        int i = 0;
        GSList *s_iter = NULL;
        *exports = lsmNfsExportRecordAllocArray(*count);
        if( *exports ) {
            for( s_iter = result; s_iter ; s_iter = g_slist_next(s_iter), i++) {
                (*exports)[i] = lsmNfsExportRecordCopy(
                                                (lsmNfsExportPtr)s_iter->data);

                if( !(*exports)[i] ) {
                    rc = LSM_ERR_NO_MEMORY;
                    lsmNfsExportRecordFreeArray(*exports, i);
                    *exports = NULL;
                    *count = 0;
                    break;
                }
            }
        } else {
            rc = LSM_ERR_NO_MEMORY;
        }
    }

    if( result ) {
        g_slist_free(result);
        result = NULL;
    }

    return rc;
}

static int nfs_export_create( lsmPluginPtr c,
                                        const char *fs_id,
                                        const char *export_path,
                                        lsmStringListPtr root_list,
                                        lsmStringListPtr rw_list,
                                        lsmStringListPtr ro_list,
                                        uint64_t anon_uid,
                                        uint64_t anon_gid,
                                        const char *auth_type,
                                        const char *options,
                                        lsmNfsExportPtr *exported,
                                        lsmFlag_t flags
                                        )
{
    int rc = LSM_ERR_OK;
    struct plugin_data *pd = (struct plugin_data*)lsmGetPrivateData(c);

    struct allocated_fs *fs = g_hash_table_lookup(pd->fs, fs_id);
    if( fs ) {
        char *key = strdup(md5(export_path));
        *exported = lsmNfsExportRecordAlloc(md5(export_path),
                                            fs_id,
                                            export_path,
                                            auth_type,
                                            root_list,
                                            rw_list,
                                            ro_list,
                                            anon_uid,
                                            anon_gid,
                                            options);

        lsmNfsExportPtr value = lsmNfsExportRecordCopy(*exported);

        if( key && *exported && value ) {
            g_hash_table_insert(fs->exports, key, value);
        } else {
            rc = LSM_ERR_NO_MEMORY;
            free(key);
            lsmNfsExportRecordFree(*exported);
            lsmNfsExportRecordFree(value);
        }

    } else {
        rc = lsmLogErrorBasic(c, LSM_ERR_NOT_FOUND_FS, "fs not found");
    }
    return rc;
}

static int nfs_export_remove( lsmPluginPtr c, lsmNfsExportPtr e,
                                lsmFlag_t flags )
{
    int rc = LSM_ERR_OK;
    struct plugin_data *pd = (struct plugin_data*)lsmGetPrivateData(c);

    struct allocated_fs *fs = g_hash_table_lookup(pd->fs,
                                                    lsmNfsExportFsIdGet(e));
    if( fs ) {
        if( !g_hash_table_remove(fs->exports, lsmNfsExportIdGet(e))) {
            rc = lsmLogErrorBasic(c, LSM_ERR_NOT_FOUND_NFS_EXPORT,
                                        "export not found");
        }
    } else {
        rc = lsmLogErrorBasic(c, LSM_ERR_NOT_FOUND_FS, "fs not found");
    }
    return rc;
}

static struct lsmNasOps nfsOps = {
    nfs_auth_types,
    nfs_export_list,
    nfs_export_create,
    nfs_export_remove
};


void free_group_grant_hash(void *v)
{
    g_hash_table_destroy((GHashTable *)v);
}

void free_allocated_fs(void *v)
{
    free_fs_record((struct allocated_fs*)v);
}

int load( lsmPluginPtr c, xmlURIPtr uri, const char *password,
                        uint32_t timeout,  lsmFlag_t flags)
{
    struct plugin_data *data = (struct plugin_data *)
                                malloc(sizeof(struct plugin_data));
    int rc = LSM_ERR_NO_MEMORY;
    if( data ) {
        memset(data, 0, sizeof(struct plugin_data));

        data->num_systems = 1;
        data->system[0] = lsmSystemRecordAlloc(sys_id,
                                                "LSM simulated storage plug-in");

        data->num_pools = MAX_POOLS;
        data->pool[0] = lsmPoolRecordAlloc("POOL_0", "POOL_ZERO", UINT64_MAX,
                                            UINT64_MAX, sys_id);
        data->pool[1] = lsmPoolRecordAlloc("POOL_1", "POOL_ONE", UINT64_MAX,
                                            UINT64_MAX, sys_id);
        data->pool[2] = lsmPoolRecordAlloc("POOL_2", "POOL_TWO", UINT64_MAX,
                                            UINT64_MAX, sys_id);
        data->pool[3] = lsmPoolRecordAlloc("POOL_3", "lsm_test_aggr",
                                            UINT64_MAX, UINT64_MAX, sys_id);

        data->access_groups = g_hash_table_new_full(g_str_hash, g_str_equal,
                                                    free, free_allocated_ag);

        /*  We will delete the key, but the value will get cleaned up in its
            own container */
        data->group_grant = g_hash_table_new_full(g_str_hash, g_str_equal,
                            free, free_group_grant_hash);

        data->fs = g_hash_table_new_full(g_str_hash, g_str_equal, free,
                                            free_allocated_fs);

        data->jobs = g_hash_table_new_full(g_str_hash, g_str_equal, free,
                                            free_allocated_job);

        if( !data->system[0] || !data->pool[0] || !data->pool[1] ||
            !data->pool[2] || !data->pool[3] || !data->access_groups ||
            !data->group_grant || !data->fs) {
            rc = LSM_ERR_NO_MEMORY;
        } else {
            rc = lsmRegisterPlugin( c, name, version, data, &mgmOps,
                                    &sanOps, &fsOps, &nfsOps);
        }
    }
    return rc;
}

int unload( lsmPluginPtr c, lsmFlag_t flags)
{
    uint32_t i = 0;

    struct plugin_data *pd = (struct plugin_data*)lsmGetPrivateData(c);

    g_hash_table_destroy(pd->jobs);
    pd->jobs = NULL;

    g_hash_table_destroy(pd->fs);
    pd->fs = NULL;

    g_hash_table_destroy(pd->group_grant);
    pd->group_grant = NULL;

    g_hash_table_destroy(pd->access_groups);

    for( i = 0; i < pd->num_volumes; ++i ) {
        lsmVolumeRecordFree(pd->volume[i].v);
        pd->volume[i].v = NULL;
        pd->volume[i].p = NULL;
    }
    pd->num_volumes = 0;

    for( i = 0; i < pd->num_pools; ++i ) {
        lsmPoolRecordFree(pd->pool[i]);
        pd->pool[i]= NULL;
    }
    pd->num_pools = 0;

    for( i = 0; i < pd->num_systems; ++i ) {
        lsmSystemRecordFree(pd->system[i]);
        pd->system[i]= NULL;
    }
    pd->num_systems = 0;

    free(pd);

    return LSM_ERR_OK;
}

int main(int argc, char *argv[] )
{
    return lsmPluginInit(argc, argv, load, unload);
}


#ifdef  __cplusplus
}
#endif
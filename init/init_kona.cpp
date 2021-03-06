/*
   Copyright (c) 2013-2015, The Linux Foundation. All rights reserved.

   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions are
   met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above
      copyright notice, this list of conditions and the following
      disclaimer in the documentation and/or other materials provided
      with the distribution.
    * Neither the name of The Linux Foundation nor the names of its
      contributors may be used to endorse or promote products derived
      from this software without specific prior written permission.

   THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
   WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
   MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
   ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
   BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
   CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
   SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
   BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
   WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
   OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
   IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <private/android_filesystem_config.h>
#include "vendor_init.h"
#include "property_service.h"
#include "android/log.h"
//#include "util.h"

#include <android-base/logging.h>
#include <android-base/properties.h>
#define _REALLY_INCLUDE_SYS__SYSTEM_PROPERTIES_H_
#include <sys/_system_properties.h>
  
#include "init_kona.h"

#include <sys/resource.h>

#define SOC_ID_PATH1     "/sys/devices/soc0/soc_id"
#define SOC_ID_PATH2     "/sys/devices/system/soc/soc0/id"
#define SOC_VER_PATH1    "/sys/devices/soc0/platform_version"
#define SOC_VER_PATH2    "/sys/devices/system/soc/soc0/platform_version"
#define BOARD_TYPE_PATH1 "/sys/devices/soc0/hw_platform"
#define BOARD_TYPE_PATH2 "/sys/devices/system/soc/soc0/hw_platform"

#define BUF_SIZE         64
#define STRCONV_(x)      #x
#define STRCONV(x)       "%" STRCONV_(x) "s"

static unsigned long msm_id;
static unsigned long msm_ver;
static char board_type[BUF_SIZE];
static char tmp[BUF_SIZE];
// sys and dev fb paths
char sys_fb_path[]  = "/sys/class/graphics/";
char dev_fb_path[]  = "/dev/graphics/";
#define DEV_GFX_HDMI "/dev/graphics/hdmi"

using android::init::property_set;

__attribute__ ((weak))
void init_msm_properties(unsigned long soc, unsigned long socrev, char *board)
{
    UNUSED(soc);
    UNUSED(socrev);
    UNUSED(board);
}

int read_file2(const char *fname, char *data, int max_size)
{
    int fd, rc;

    if (max_size < 1)
        return 0;

    fd = open(fname, O_RDONLY);
    if (fd < 0) {
        //ERROR("failed to open '%s'\n", fname);
        return 0;
    }

    rc = read(fd, data, max_size - 1);
    if ((rc > 0) && (rc < max_size))
        data[rc] = '\0';
    else
        data[0] = '\0';
    close(fd);

    return 1;
}

/*
 * setPerms: sets the permission to the file node
 * @path: file node
 * @mode: permissions to be set(0664)
 */
void setPerms(char *path, uint32_t mode)
{
    // set the permission if the file exists
    int fd = open(path, O_RDONLY | O_NOFOLLOW);
    if (fd >= 0) {
       if (fchmod(fd, mode) < 0)
          //ERROR("chmod failed for %s: errno = %d", path, errno);
       close(fd);
    }
}

/*
 * setOwners: sets the owner and group for a file node
 * @path: file node
 * @owner: AID for owner
 * @group: AID for group
 */
void setOwners(char *path, int owner, int group)
{
    // set the permissinn if the file exists
    int fd = open(path, O_RDONLY | O_NOFOLLOW);
    if (fd >= 0) {
       if (fchown(fd, owner, group) < 0)
          //ERROR(" chown failed for %s: errno = %d", path, errno);
       close(fd);
    }
}

/*
 * Setup Display related nodes & permissions. For HDMI, it can be fb1 or fb2
 * Loop through the sysfs nodes and determine the HDMI(dtv panel)
 */
void set_display_node_perms()
{
    char panel_type[] = "dtv panel";
    char buf[BUF_SIZE];
    int num;

    for (num=0; num<=2; num++) {
        snprintf(tmp,sizeof(tmp),"%sfb%d/msm_fb_type", sys_fb_path, num);
        if(read_file2(tmp, buf, sizeof(buf))) {
            if(!strncmp(buf, panel_type, strlen(panel_type))) {
                // Set appropriate permissions for the nodes
                snprintf(tmp, sizeof(tmp), "%sfb%d/hpd", sys_fb_path, num);
                setPerms(tmp, 0664);
                setOwners(tmp, AID_SYSTEM, AID_GRAPHICS);
                snprintf(tmp, sizeof(tmp), "%sfb%d/res_info", sys_fb_path, num);
                setPerms(tmp, 0664);
                setOwners(tmp, AID_SYSTEM, AID_GRAPHICS);
                snprintf(tmp, sizeof(tmp), "%sfb%d/vendor_name", sys_fb_path,
                             num);
                setPerms(tmp, 0664);
                setOwners(tmp, AID_SYSTEM, AID_GRAPHICS);
                snprintf(tmp, sizeof(tmp), "%sfb%d/product_description",
                            sys_fb_path, num);
                setPerms(tmp, 0664);
                setOwners(tmp, AID_SYSTEM, AID_GRAPHICS);
                snprintf(tmp, sizeof(tmp), "%sfb%d/video_mode",
                            sys_fb_path, num);
                setPerms(tmp, 0664);
                snprintf(tmp, sizeof(tmp), "%sfb%d/format_3d", sys_fb_path,
                            num);
                setPerms(tmp, 0664);
                setOwners(tmp, AID_SYSTEM, AID_SYSTEM);
                snprintf(tmp, sizeof(tmp), "%sfb%d/hdcp/tp", sys_fb_path, num);
                setPerms(tmp, 0664);
                setOwners(tmp, AID_SYSTEM, AID_SYSTEM);
                snprintf(tmp, sizeof(tmp), "%sfb%d/cec/enable", sys_fb_path, num);
                setPerms(tmp, 0664);
                setOwners(tmp, AID_SYSTEM, AID_SYSTEM);
                snprintf(tmp, sizeof(tmp), "%sfb%d/cec/logical_addr", sys_fb_path, num);
                setPerms(tmp, 0664);
                setOwners(tmp, AID_SYSTEM, AID_SYSTEM);
                snprintf(tmp, sizeof(tmp), "%sfb%d/cec/rd_msg", sys_fb_path, num);
                setPerms(tmp, 0664);
                setOwners(tmp, AID_SYSTEM, AID_SYSTEM);
                snprintf(tmp, sizeof(tmp), "%sfb%d/cec/wr_msg", sys_fb_path, num);
                setPerms(tmp, 0600);
                setOwners(tmp, AID_SYSTEM, AID_SYSTEM);
                snprintf(tmp, sizeof(tmp), "%sfb%d", dev_fb_path, num);
                symlink(tmp, DEV_GFX_HDMI);
                break;
            }
        }
    }
    // Set the permission for idle_time.
    snprintf(tmp, sizeof(tmp), "%sfb0/idle_time", sys_fb_path);
    setPerms(tmp, 0664);
    setOwners(tmp, AID_SYSTEM, AID_GRAPHICS);

    // Set the permission for mdp/bw_mode_bitmap.
    snprintf(tmp, sizeof(tmp), "%sfb0/mdp/bw_mode_bitmap", sys_fb_path);
    setPerms(tmp, 0664);
    setOwners(tmp, AID_SYSTEM, AID_GRAPHICS);

    // Set write permission for dynamic_fps node.
    snprintf(tmp, sizeof(tmp), "%sfb0/dynamic_fps", sys_fb_path);
    setPerms(tmp, 0664);
    setOwners(tmp, AID_SYSTEM, AID_GRAPHICS);
    // Set permissions for dynamic partial update
    snprintf(tmp, sizeof(tmp), "%sfb0/dyn_pu", sys_fb_path);
    setPerms(tmp, 0664);
    setOwners(tmp, AID_SYSTEM, AID_GRAPHICS);
    // Set permissions for cmd auto refresh
    snprintf(tmp, sizeof(tmp), "%sfb0/msm_cmd_autorefresh_en", sys_fb_path);
    setPerms(tmp, 0664);
    setOwners(tmp, AID_SYSTEM, AID_GRAPHICS);
}

/*
static int check_rlim_action()
{
    char pval[PROP_VALUE_MAX];
    int rc;
    struct rlimit rl;
    rc = property_get("persist.debug.trace",pval);

    if(rc && (strcmp(pval,"1") == 0)) {
        rl.rlim_cur = RLIM_INFINITY;
        rl.rlim_max = RLIM_INFINITY;
        if (setrlimit(RLIMIT_CORE, &rl) < 0) {
            ERROR("could not enable core file generation");
        }
    }
    return 0;
}
*/
void load_lmi() {
      property_set("ro.product.model", "Redmi K30 Pro");
      property_set("ro.build.product", "lmi");
      property_set("ro.product.device", "lmi");
      property_set("ro.build.description", "qssi-user 10 QKQ1.191117.002 V11.0.16.0.QJKCNXM release-keys");
      property_set("ro.control_privapp_permissions", "log");
	  property_set("ro.vendor.build.fingerprint", "Redmi/lmi/lmi:10/QKQ1.191117.002/V11.0.16.0.QJKCNXM:user/release-keys");
      property_set("ro.build.fingerprint", "Redmi/lmi/lmi:10/QKQ1.191117.002/V11.0.16.0.QJKCNXM:user/release-keys");
}
void vendor_load_properties()
{

	/* Define MSM family properties */
    init_msm_properties(msm_id, msm_ver, board_type);

    /*Check for kgsl node and disable HW composition*/
    if (access("/dev/kgsl-3d0", F_OK) < 0) {
        //ERROR("Failed to open kgsl node, falling back to SW OpenGL err:%s", strerror(errno));
        property_set("persist.sys.force_sw_gles", "1");
    } else {
        property_set("persist.sys.force_sw_gles", "0");
    }

	load_lmi();

    /* Set Display Node Permissions */
    set_display_node_perms();

    /*check for coredump*/
    //check_rlim_action();
}

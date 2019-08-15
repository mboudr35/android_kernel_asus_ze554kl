/*
 *
 * FocalTech fts TouchScreen driver.
 *
 * Copyright (c) 2010-2016, Focaltech Ltd. All rights reserved.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

/*****************************************************************************
*
* File Name: focaltech_upgrade_ft8716.c
*
* Author:    fupeipei
*
* Created:    2016-08-15
*
* Abstract:
*
* Reference:
*
*****************************************************************************/

/*****************************************************************************
* 1.Included header files
*****************************************************************************/
#include "../focaltech_core.h"
#include "../focaltech_flash.h"
#include "focaltech_upgrade_common.h"

/*****************************************************************************
* Static variables
*****************************************************************************/
#define APP_FILE_MAX_SIZE           (60 * 1024)
#define APP_FILE_MIN_SIZE           (8)
#define APP_FILE_VER_MAPPING        (0x10E)
#define APP_FILE_VENDORID_MAPPING   (0x10C)
#define APP_FILE_CHIPID_MAPPING     (0x11E)
#define CONFIG_VENDOR_ID_ADDR       (0x04)
#define CONFIG_PROJECT_ID_ADDR      (0x20)

/*****************************************************************************
* Global variable or extern global variabls/functions
*****************************************************************************/
static int fts_ctpm_get_app_i_file_ver(void);
static int fts_ctpm_get_app_bin_file_ver(char *firmware_name);
static int fts_ctpm_fw_upgrade_with_app_i_file(struct i2c_client *client);
static int fts_ctpm_fw_upgrade_with_app_bin_file(struct i2c_client *client, char *firmware_name);

struct fts_Upgrade_Fun fts_updatefun =
{
    .chip_idh = 0x87,
    .chip_idl = 0x16,
    .app_valid = true,
    .need_upgrade = true,
    .get_projectid = false,
    .project_id = {0},
    .get_app_bin_file_ver = fts_ctpm_get_app_bin_file_ver,
    .get_app_i_file_ver = fts_ctpm_get_app_i_file_ver,
    .upgrade_with_app_i_file = fts_ctpm_fw_upgrade_with_app_i_file,
    .upgrade_with_app_bin_file = fts_ctpm_fw_upgrade_with_app_bin_file,
    .upgrade_with_lcd_cfg_i_file = NULL,
    .upgrade_with_lcd_cfg_bin_file = NULL,
};

/*****************************************************************************
* Static function prototypes
*****************************************************************************/

/************************************************************************
* Name: fts_ctpm_get_app_bin_file_ver
* Brief:  get .i file version
* Input: no
* Output: no
* Return: fw version
***********************************************************************/
static int fts_ctpm_get_app_bin_file_ver(char *firmware_name)
{
    u8 *pbt_buf = NULL;
    int fwsize = fts_GetFirmwareSize(firmware_name);

    FTS_FUNC_ENTER();

    pr_err("start upgrade with app.bin!! \n");

    if (fwsize <= 0)
    {
        pr_err("[UPGRADE]: Get firmware size failed!! \n");
        return -EIO;
    }

    if (fwsize < APP_FILE_MIN_SIZE || fwsize > APP_FILE_MAX_SIZE)
    {
        pr_err("[UPGRADE]: FW length error, upgrade fail!! \n");
        return -EIO;
    }

    pbt_buf = (unsigned char *)kmalloc(fwsize + 1, GFP_ATOMIC);
    if (fts_ReadFirmware(firmware_name, pbt_buf))
    {
        pr_err("[UPGRADE]: request_firmware failed!! \n");
        kfree(pbt_buf);
        return -EIO;
    }

    FTS_FUNC_EXIT();

    return pbt_buf[APP_FILE_VER_MAPPING];
}

/************************************************************************
* Name: fts_ctpm_get_app_i_file_ver
* Brief:  get .i file version
* Input: no
* Output: no
* Return: fw version
***********************************************************************/
static int fts_ctpm_get_app_i_file_ver(void)
{
    return CTPM_FW[APP_FILE_VER_MAPPING];
}

/************************************************************************
* Name: fts_ctpm_get_vendor_id
* Brief: no
* Input: no
* Output: no
* Return: no
***********************************************************************/
static int fts_ctpm_get_vendor_id(struct i2c_client * client)
{
    int i_ret = 0;
    u8 vendorid[4] = {0};
    u8 auc_i2c_write_buf[10];
    u8 i = 0;

#if (!FTS_GET_VENDOR_ID_FROM_FLASH)
    return 0;
#endif

    if (fts_updatefun_curr.app_valid == false)
    {
        for (i = 0; i < FTS_UPGRADE_LOOP; i++)
        {
            /*read project code*/
            auc_i2c_write_buf[0] = 0x03;
            auc_i2c_write_buf[1] = 0x00;

            auc_i2c_write_buf[2] = (u8)((CONFIG_VENDOR_ID_ADDR-1)>>8);
            auc_i2c_write_buf[3] = (u8)(CONFIG_VENDOR_ID_ADDR-1);
            i_ret = fts_i2c_read(client, auc_i2c_write_buf, 4, vendorid, 2);
            if (i_ret < 0)
            {
                pr_err("[fts][touch][UPGRADE]: read flash : i_ret = %d!!", i_ret);
                continue;
            }

            break;
        }

        if (i >= FTS_UPGRADE_LOOP)
        {
            pr_err("[fts][touch][UPGRADE]: read vendor id from flash more than 30 times!!");
            return -EIO;
        }

        if ((vendorid[1] != FTS_VENDOR_1_ID) && (vendorid[1] != FTS_VENDOR_2_ID))
        {
            fts_updatefun_curr.need_upgrade = false;
            pr_err("[fts][touch][UPGRADE]: vendor id error : 0x%x, can not upgrade!!", vendorid[1]);
            return -EIO;
        }

        /*read vendor id ok, and app is invalid, need upgrade*/
        fts_updatefun_curr.need_upgrade = true;

        pr_err("[fts][touch][UPGRADE]: vendor id : 0x%x!!", vendorid[1]);
    }
    return 0;
}

/************************************************************************
* Name: fts_ctpm_get_project_id
* Brief: no
* Input: no
* Output: no
* Return: no
***********************************************************************/
static int fts_ctpm_get_project_id(struct i2c_client * client)
{
    int i_ret = 0;
    u8 projectid[0x21] = {0};
    u8 auc_i2c_write_buf[10];
    u8 i = 0;

    if (fts_updatefun_curr.get_projectid)
    {
        for (i = 0; i < FTS_UPGRADE_LOOP; i++)
        {
            /*read project code*/
            auc_i2c_write_buf[0] = 0x03;
            auc_i2c_write_buf[1] = 0x00;

            auc_i2c_write_buf[2] = (u8)((CONFIG_PROJECT_ID_ADDR-1)>>8);
            auc_i2c_write_buf[3] = (u8)(CONFIG_PROJECT_ID_ADDR-1);
            i_ret = fts_i2c_read(client, auc_i2c_write_buf, 4, projectid, 0x21);
            if (i_ret < 0)
            {
                FTS_DEBUG("[UPGRADE]: read flash : i_ret = %d!!", i_ret);
                continue;
            }

            break;
        }

        if (i >= FTS_UPGRADE_LOOP)
        {
            FTS_ERROR( "[UPGRADE]: read project id from flash more than 30 times!!");
            return -EIO;
        }

        for (i=0; i<0x20; i++)
        {
            fts_updatefun_curr.project_id[i] = projectid[i+1];
        }

        FTS_DEBUG("[UPGRADE]: project id : %s!!", fts_updatefun_curr.project_id);
    }
    return 0;
}

/************************************************************************
* Name: fts_ctpm_write_pram
* Brief: fw upgrade
* Input: i2c info, file buf, file len
* Output: no
* Return: fail <0
***********************************************************************/
static int fts_ctpm_write_pram(struct i2c_client * client, u8* pbt_buf, u32 dw_lenth)
{
    int i_ret;
    bool inrom = false;

    FTS_FUNC_ENTER();

    /*check the length of the pramboot*/
    FTS_DEBUG("[UPGRADE]: pramboot length: %d!!", dw_lenth);
    if (dw_lenth > 0x10000 || dw_lenth ==0)
    {
        return -EIO;
    }

    /*send comond to FW, reset and start write pramboot*/
    i_ret = fts_ctpm_start_fw_upgrade(client);
    if (i_ret < 0)
    {
        FTS_ERROR( "[UPGRADE]: send upgrade cmd to FW error!!");
        return i_ret;
    }

    /*check run in rom or not! if run in rom, will write pramboot*/
    inrom = fts_ctpm_check_in_rom(client);
    if (!inrom)
    {
        FTS_ERROR( "[UPGRADE]: not run in rom, write pramboot fail!!");
        return -EIO;
    }

    /*write pramboot to pram*/
    i_ret = fts_ctpm_write_pramboot_for_idc(client, dw_lenth, aucFW_PRAM_BOOT);
    if (i_ret < 0)
    {
        return i_ret;
        FTS_ERROR( "[UPGRADE]: write pramboot fail!!");
    }

    /*read out checksum*/
    i_ret = fts_ctpm_pramboot_ecc(client);
    if (i_ret < 0)
    {
        return i_ret;
        FTS_ERROR( "[UPGRADE]: write pramboot ecc error!!");
    }

    /*start pram*/
    fts_ctpm_start_pramboot(client);

    FTS_FUNC_EXIT();

    return 0;
}

/************************************************************************
* Name: fts_ctpm_write_app
* Brief:  fw upgrade
* Input: i2c info, file buf, file len
* Output: no
* Return: fail <0
***********************************************************************/
static int fts_ctpm_write_app(struct i2c_client * client, u8* pbt_buf, u32 dw_lenth)
{
    u32 temp;
    int i_ret;
    bool inpram = false;

    FTS_FUNC_ENTER();

    /*check run in pramboot or not! if not rum in pramboot, can not upgrade*/
    inpram = fts_ctpm_check_in_pramboot(client);
    if (!inpram)
    {
        pr_err("[fts][touch][UPGRADE]: not run in pram, upgrade fail!!");
        return -EIO;
    }

    /*upgrade init*/
    i_ret = fts_ctpm_upgrade_idc_init(client, dw_lenth);
    if (i_ret < 0)
    {
        pr_err("[fts][touch][UPGRADE]: upgrade init error, upgrade fail!!");
        return i_ret;
    }

    /*read vendor id from flash, if vendor id error, can not upgrade*/
    i_ret = fts_ctpm_get_vendor_id(client);
    if (i_ret < 0)
    {
        pr_err("[fts][touch][UPGRADE]: read vendor id in flash fail!!");
        return i_ret;
    }

    /*read project id from flash*/
    i_ret = fts_ctpm_get_project_id(client);
    if (i_ret < 0)
    {
        pr_err("[fts][touch][UPGRADE]: read project id in flash fail!!");
        return i_ret;
    }

    if (!fts_updatefun_curr.need_upgrade)
    {
        pr_err("[fts][touch][UPGRADE]: no need upgrade!!");

        fts_ctpm_rom_or_pram_reset(client, false);
        return 0;
    }

    /*erase the app erea in flash*/
    i_ret = fts_ctpm_erase_flash(client);
    if (i_ret < 0)
    {
        pr_err("[fts][touch][UPGRADE]: erase flash error!!");
        return i_ret;
    }

    /*start to write app*/
    i_ret = fts_ctpm_write_app_for_idc(client, dw_lenth, pbt_buf);
    if (i_ret < 0)
    {
        pr_err("[fts][touch][UPGRADE]: write app error!!");
        return i_ret;
    }

    /*read check sum*/
    temp = 0x1000;
    i_ret = fts_ctpm_upgrade_ecc(client, temp, dw_lenth);
    if (i_ret < 0)
    {
        pr_err("[fts][touch][UPGRADE]: ecc error!!");
        return i_ret;
    }

    /*upgrade success, reset the FW*/
    fts_ctpm_rom_or_pram_reset(client, false);

    FTS_FUNC_EXIT();

    return 0;
}

/************************************************************************
* Name: fts_ctpm_fw_upgrade_use_buf
* Brief: upgrade with *.bin file
* Input: i2c info, file name
* Output: no
* Return: success =0
***********************************************************************/
static int fts_ctpm_fw_upgrade_use_buf(struct i2c_client *client, u8* pbt_buf, u32 fwsize)
{
    int i_ret=0;

    int fw_len;

    pr_err("[fts][touch][UPGRADE]: write pram now!!");

    /*write pramboot*/
    fw_len = fts_getsize(PRAMBOOT_SIZE);
    pr_err("[fts][touch][UPGRADE]: pramboot size : %d!! \n", fw_len);
    i_ret = fts_ctpm_write_pram(client, aucFW_PRAM_BOOT, fw_len);
    if (i_ret != 0)
    {
        pr_err("[fts][touch][UPGRADE]: write pram failed!!");
        return -EIO;
    }

    /*write app*/
    i_ret =  fts_ctpm_write_app(client, pbt_buf, fwsize);
    if (i_ret != 0)
    {
        pr_err("[fts][touch][UPGRADE]: upgrade failed!!");
    }
    else if (fts_updateinfo_curr.AUTO_CLB==AUTO_CLB_NEED)
    {
#if FTS_AUTO_CLB_EN
        fts_ctpm_auto_clb(client);
#endif
    }

    pr_err("[fts][touch] %s end \n",__func__);

    return i_ret;
}

/************************************************************************
* Name: fts_ctpm_fw_upgrade_with_app_i_file
* Brief:  upgrade with *.i file
* Input: i2c info
* Output: no
* Return: fail <0
***********************************************************************/
static int fts_ctpm_fw_upgrade_with_app_i_file(struct i2c_client *client)
{
    int i_ret=0;
    u32 fw_len;

    pr_err("[fts][touch]start upgrade app.i \n");

    fw_len = fts_getsize(FW_SIZE);

    i_ret = fts_ctpm_fw_upgrade_use_buf(client, CTPM_FW, fw_len);

    pr_err("[fts][touch]end upgrade app.i \n");

    return i_ret;
}

/************************************************************************
* Name: fts_ctpm_fw_upgrade_with_app_bin_file
* Brief: upgrade with *.bin file
* Input: i2c info, file name
* Output: no
* Return: success =0
***********************************************************************/
static int fts_ctpm_fw_upgrade_with_app_bin_file(struct i2c_client *client, char *firmware_name)
{
    u8 *pbt_buf = NULL;
    int i_ret=0;
    int fwsize = fts_GetFirmwareSize(firmware_name);
    bool ecc_ok = false;

    FTS_INFO("[UPGRADE]: start upgrade with app.bin!!");

    if (fwsize <= 0)
    {
        FTS_ERROR("[UPGRADE]: Get firmware size failed!!");
        return -EIO;
    }
    if (fwsize < APP_FILE_MIN_SIZE || fwsize > APP_FILE_MAX_SIZE)
    {
        FTS_ERROR("[UPGRADE]: FW length error!!");
        return -EIO;
    }

    /*fw upgrade*/
    pbt_buf = (unsigned char *)kmalloc(fwsize + 1, GFP_ATOMIC);
    if (fts_ReadFirmware(firmware_name, pbt_buf))
    {
        FTS_ERROR("[UPGRADE]: request_firmware failed!!");
        kfree(pbt_buf);
        return -EIO;
    }

    if ((pbt_buf[0x10C] != FTS_VENDOR_1_ID) && (pbt_buf[0x10C] != FTS_VENDOR_2_ID))
    {
        FTS_ERROR("[UPGRADE]: vendor id is error, app.bin upgrade failed!!");
        kfree(pbt_buf);
        return -EIO;
    }

    if (pbt_buf[0x11E] != chip_types.chip_idh)
    {
        FTS_ERROR("[UPGRADE]: chip id is error, app.bin upgrade failed!!");
        kfree(pbt_buf);
        return -EIO;
    }

    /*check the app.bin invalid or not*/
    ecc_ok = fts_check_app_bin(pbt_buf, fwsize);

    if (ecc_ok)
    {
        FTS_INFO("[UPGRADE]: app.bin ecc ok!!");
        i_ret = fts_ctpm_fw_upgrade_use_buf(client, pbt_buf, fwsize);
        if (i_ret != 0)
        {
            FTS_ERROR("[UPGRADE]: upgrade failed");
            kfree(pbt_buf);
            return -EIO;
        }
        else
        {
#if FTS_AUTO_CLB_EN
            fts_ctpm_auto_clb(client);  /*start auto CLB*/
#endif
        }
    }
    else
    {
        FTS_ERROR("[UPGRADE]: app.bin ecc failed!!");
        kfree(pbt_buf);
        return -EIO;
    }

    kfree(pbt_buf);

    return i_ret;
}


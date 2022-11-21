/***********************************************************
*  File: tuya_gpio_test.c
*  Author: lql
*  Date: 20180502
***********************************************************/

#include <stdio.h>
#include <stdlib.h>

#include "tuya_os_adapter.h"
#include "tuya_gpio.h"
#include "uni_log.h"
#include "gpio_test.h"


typedef struct
{
    UINT_T  ionum;
    TY_GPIO_PORT_E iopin[8];
}CTRL_GROUP;

typedef struct
{
    UINT_T group_num;
    CTRL_GROUP group[7];
}GPIO_TEST_TABLE;

STATIC GPIO_TEST_TABLE gpio_test_table = {
    .group_num = 0,
    .group = { { .ionum = 0 } }
};

BOOL_T gpio_test_spcl_cb(UINT_T idx)
{
    UINT_T i,j;

    for(i = 0; i < gpio_test_table.group[idx].ionum; i++) {
        //set io direction
        
        for(j = 0; j < gpio_test_table.group[idx].ionum; j++) {
            if(i == j) {
                tuya_gpio_inout_set(gpio_test_table.group[idx].iopin[j],FALSE);
            } else {
                tuya_gpio_inout_set(gpio_test_table.group[idx].iopin[j],TRUE);
                
            }
        }
        // write 1
        tuya_gpio_write(gpio_test_table.group[idx].iopin[i],TRUE);
        for(j = 0; j < gpio_test_table.group[idx].ionum; j++) {
            if(i!= j) {
                if(tuya_gpio_read(gpio_test_table.group[idx].iopin[j]) != 1) {
                    PR_ERR("[%d]gpio test err_high i = %d,j = %d",idx,i,j);
                    return FALSE;
                }
            }
        }

        // write 0
        tuya_gpio_write(gpio_test_table.group[idx].iopin[i],FALSE);
        for(j = 0; j < gpio_test_table.group[idx].ionum; j++) {
            if(i!= j) {
                if(tuya_gpio_read(gpio_test_table.group[idx].iopin[j]) != 0) {
                    PR_ERR("[%d]gpio test err_lowi = %d,j = %d",idx,i,j);
                    return FALSE;
                }
            }
        }
        
    }

    return TRUE;
}



BOOL_T gpio_test_all(IN CONST CHAR_T *in, OUT CHAR_T *out)
{    
    UCHAR_T str_len = 0;
    UCHAR_T group_cnt = 0;
    UCHAR_T io_cnt = 0;
    BOOL_T pair_flag = 0;
    CHAR_T *ptemp = NULL;
    CHAR_T *pstart = NULL;  
    CHAR_T ctempbuf[16] = {0};
    
    if(in == NULL) {
        return FALSE;
    }

    str_len = strlen(in);
    while(str_len > 0) {
        ptemp = strstr(in, "\"G\":[");
        if(ptemp != NULL) {
            break;
        }
        ptemp++;
        str_len--;
    }
    
    if(str_len <= 0) {    /* can't find !! */
        return FALSE;
    }

    ptemp += strlen("{\"G\":[") - 1;
    
    while(*ptemp != ']') {
        if(pair_flag) {
            if(*ptemp == ',') {
                strncpy(ctempbuf, pstart, ((ptemp - pstart) * SIZEOF(CHAR_T)));
                gpio_test_table.group[group_cnt].iopin[io_cnt] = atoi(ctempbuf);
                memset(ctempbuf, 0, SIZEOF(ctempbuf));
                pstart = ptemp + 1;
                io_cnt++;
            } else if(*ptemp == '"') {
                strncpy(ctempbuf, pstart, ((ptemp - pstart) * SIZEOF(CHAR_T)));
                gpio_test_table.group[group_cnt].iopin[io_cnt] = atoi(ctempbuf);
                memset(ctempbuf, 0, SIZEOF(ctempbuf));
                gpio_test_table.group[group_cnt].ionum = io_cnt + 1;
                pair_flag = FALSE;
                group_cnt++;
                io_cnt = 0;
            }
    
        } else {
            if(*ptemp == '"') {
                pair_flag = TRUE;
                pstart = ptemp + 1; 
            }
        }
        ptemp++;
    }

    gpio_test_table.group_num = group_cnt;

    UINT_T idx, i;
    BOOL_T result = TRUE;
    BOOL_T ret;
    ptemp = out;
    for(idx = 0; idx < gpio_test_table.group_num; idx++) {
        ret = gpio_test_spcl_cb(idx);
        if(FALSE == ret) {
            result = FALSE;
            
            if(strlen(out) != 0)
                *ptemp ++ = ',';
            else {
                strcpy(ptemp, "\"G\":[");
                ptemp += strlen(out);
            }
            *ptemp ++ = '\"';

            for(i = 0; i < gpio_test_table.group[idx].ionum; i++) {
                ptemp += sprintf(ptemp, "%d", gpio_test_table.group[idx].iopin[i]);
                *ptemp ++ = ',';
            }
            ptemp --;
            *ptemp ++ = '\"';
        }
    }

    if(FALSE == result)
        *ptemp = ']';
        
    return result;
    
}


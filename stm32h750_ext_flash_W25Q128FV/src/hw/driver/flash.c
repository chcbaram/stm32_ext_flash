/*
 * flash.c
 *
 *  Created on: 2020. 1. 24.
 *      Author: Baram
 */





#include "flash.h"
#include "qspi.h"


#define FLASH_MAX_SECTOR          1


typedef struct
{
  int16_t  index;
  uint32_t bank;
  uint32_t addr;
  uint32_t length;
} flash_tbl_t;


flash_tbl_t flash_tbl_bank1[FLASH_MAX_SECTOR] =
    {
        {0, FLASH_BANK_1, 0x08000000, 128*1024},
    };




void flashCmdifInit(void);
void flashCmdif(void);





bool flashInit(void)
{

#ifdef _USE_HW_CMDIF
  flashCmdifInit();
#endif

  flash_tbl_bank1[0].index = 0;
  flash_tbl_bank1[0].bank = FLASH_BANK_1;
  flash_tbl_bank1[0].addr = 0x08000000;
  flash_tbl_bank1[0].length = 128*1024;

  return true;
}

bool flashErase(uint32_t addr, uint32_t length)
{
  bool ret = false;

  int32_t start_sector = -1;
  int32_t end_sector = -1;
  uint32_t banks;
  const flash_tbl_t *flash_tbl;


#ifdef _USE_HW_QSPI
  if (addr >= qspiGetAddr() && addr < (qspiGetAddr() + qspiGetLength()))
  {
    ret = qspiErase(addr - qspiGetAddr(), length);
    return ret;
  }
#endif


  HAL_FLASH_Unlock();

  for (banks = 0; banks < 1; banks++)
  {
    start_sector = -1;
    end_sector = -1;

    if (banks == 0)
    {
      flash_tbl = flash_tbl_bank1;
    }
    else
    {
      return false;
    }

    for (int i=0; i<FLASH_MAX_SECTOR; i++)
    {
      bool update = false;
      uint32_t start_addr;
      uint32_t end_addr;


      start_addr = flash_tbl[i].addr;
      end_addr   = flash_tbl[i].addr + flash_tbl[i].length - 1;

      if (start_addr >= addr && start_addr < (addr+length))
      {
        update = true;
      }
      if (end_addr >= addr && end_addr < (addr+length))
      {
        update = true;
      }

      if (addr >= start_addr && addr <= end_addr)
      {
        update = true;
      }
      if ((addr+length-1) >= start_addr && (addr+length-1) <= end_addr)
      {
        update = true;
      }


      if (update == true)
      {
        if (start_sector < 0)
        {
          start_sector = i;
        }
        end_sector = i;
      }
    }

    if (start_sector >= 0)
    {
      FLASH_EraseInitTypeDef EraseInit;
      uint32_t SectorError;
      HAL_StatusTypeDef status;


      EraseInit.Sector       = start_sector;
      EraseInit.NbSectors    = (end_sector - start_sector) + 1;
      EraseInit.TypeErase    = FLASH_TYPEERASE_SECTORS;
      EraseInit.VoltageRange = FLASH_VOLTAGE_RANGE_4;
      EraseInit.Banks        = flash_tbl[start_sector].bank;

      status = HAL_FLASHEx_Erase(&EraseInit, &SectorError);
      if (status == HAL_OK)
      {
        ret = true;
      }
    }
  }

  HAL_FLASH_Lock();

  return ret;
}

bool flashWrite(uint32_t addr, uint8_t *p_data, uint32_t length)
{
  bool ret = true;
  uint32_t index;
  uint32_t write_length;
  uint32_t write_addr;
  static uint8_t buf[32];
  uint32_t offset;
  HAL_StatusTypeDef status;


#ifdef _USE_HW_QSPI
  if (addr >= qspiGetAddr() && addr < (qspiGetAddr() + qspiGetLength()))
  {
    ret = qspiWrite(addr - qspiGetAddr(), p_data, length);
    return ret;
  }
#endif


  HAL_FLASH_Unlock();

  index = 0;
  offset = addr%32;

  if (offset != 0 || length < 32)
  {
    write_addr = addr - offset;
    memcpy(&buf[0], (void *)write_addr, 32);
    memcpy(&buf[offset], &p_data[0], constrain(32-offset, 0, length));

    status = HAL_FLASH_Program(FLASH_TYPEPROGRAM_FLASHWORD, write_addr, (uint32_t)buf);
    if (status != HAL_OK)
    {
      return false;
    }

    if (length < 32)
    {
      index += length;
    }
    else
    {
      index += (32 - offset);
    }
  }


  while(index < length)
  {
    write_length = constrain(length - index, 0, 32);

    status = HAL_FLASH_Program(FLASH_TYPEPROGRAM_FLASHWORD, addr + index, (uint32_t)&p_data[index]);
    if (status != HAL_OK)
    {
      ret = false;
      break;
    }

    index += write_length;

    if ((length - index) > 0 && (length - index) < 32)
    {
      offset = length - index;
      write_addr = addr + index;
      memcpy(&buf[0], (void *)write_addr, 32);
      memcpy(&buf[0], &p_data[index], offset);

      status = HAL_FLASH_Program(FLASH_TYPEPROGRAM_FLASHWORD, write_addr, (uint32_t)buf);
      if (status != HAL_OK)
      {
        return false;
      }
      break;
    }
  }

  HAL_FLASH_Lock();

  return ret;
}

bool flashRead(uint32_t addr, uint8_t *p_data, uint32_t length)
{
  bool ret = true;
  uint8_t *p_byte = (uint8_t *)addr;


#ifdef _USE_HW_QSPI
  if (addr >= qspiGetAddr() && addr < (qspiGetAddr() + qspiGetLength()))
  {
    ret = qspiRead(addr - qspiGetAddr(), p_data, length);
    return ret;
  }
#endif

  for (int i=0; i<length; i++)
  {
    p_data[i] = p_byte[i];
  }

  return ret;
}





#ifdef _USE_HW_CMDIF
void flashCmdifInit(void)
{
  cmdifAdd("flash", flashCmdif);
}

void flashCmdif(void)
{
  bool ret = true;
  uint32_t i;
  uint32_t addr;
  uint32_t length;
  uint8_t  data;
  uint32_t pre_time;
  bool flash_ret;


  if (cmdifGetParamCnt() == 1)
  {
    if(cmdifHasString("info", 0) == true)
    {
      cmdifPrintf("flash addr  : 0x%X\n", 0x8000000);
      cmdifPrintf("qspi  addr  : 0x%X\n", 0x90000000);
    }
    else
    {
      ret = false;
    }
  }
  else if (cmdifGetParamCnt() == 3)
  {
    if(cmdifHasString("read", 0) == true)
    {
      addr   = (uint32_t)cmdifGetParam(1);
      length = (uint32_t)cmdifGetParam(2);

      for (i=0; i<length; i++)
      {
        flash_ret = flashRead(addr+i, &data, 1);

        if (flash_ret == true)
        {
          cmdifPrintf( "addr : 0x%X\t 0x%02X\n", addr+i, data);
        }
        else
        {
          cmdifPrintf( "addr : 0x%X\t Fail\n", addr+i);
        }
      }
    }
    else if(cmdifHasString("erase", 0) == true)
    {
      addr   = (uint32_t)cmdifGetParam(1);
      length = (uint32_t)cmdifGetParam(2);

      pre_time = millis();
      flash_ret = flashErase(addr, length);

      cmdifPrintf( "addr : 0x%X\t len : %d %d ms\n", addr, length, (millis()-pre_time));
      if (flash_ret)
      {
        cmdifPrintf("OK\n");
      }
      else
      {
        cmdifPrintf("FAIL\n");
      }
    }
    else if(cmdifHasString("write", 0) == true)
    {
      addr = (uint32_t)cmdifGetParam(1);
      data = (uint8_t )cmdifGetParam(2);

      pre_time = millis();
      flash_ret = flashWrite(addr, &data, 1);

      cmdifPrintf( "addr : 0x%X\t 0x%02X %dms\n", addr, data, millis()-pre_time);
      if (flash_ret)
      {
        cmdifPrintf("OK\n");
      }
      else
      {
        cmdifPrintf("FAIL\n");
      }
    }
    else
    {
      ret = false;
    }
  }
  else
  {
    ret = false;
  }


  if (ret == false)
  {
    cmdifPrintf( "flash info\n");
    cmdifPrintf( "flash read  [addr] [length]\n");
    cmdifPrintf( "flash erase [addr] [length]\n");
    cmdifPrintf( "flash write [addr] [data]\n");
  }

}
#endif

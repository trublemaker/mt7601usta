/*
 *************************************************************************
 * Ralink Tech Inc.
 * 5F., No.36, Taiyuan St., Jhubei City,
 * Hsinchu County 302,
 * Taiwan, R.O.C.
 *
 * (c) Copyright 2002-2010, Ralink Technology, Inc.
 *
 * This program is free software; you can redistribute it and/or modify  *
 * it under the terms of the GNU General Public License as published by  *
 * the Free Software Foundation; either version 2 of the License, or     *
 * (at your option) any later version.                                   *
 *                                                                       *
 * This program is distributed in the hope that it will be useful,       *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 * GNU General Public License for more details.                          *
 *                                                                       *
 * You should have received a copy of the GNU General Public License     *
 * along with this program; if not, write to the                         *
 * Free Software Foundation, Inc.,                                       *
 * 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 *                                                                       *
 *************************************************************************/


#include "rt_config.h"


#ifdef VIDEO_TURBINE_SUPPORT



BOOLEAN UpdateFromGlobal = FALSE;

void VideoTurbineUpdate(
	IN PRTMP_ADAPTER pAd)
{
	if (UpdateFromGlobal == TRUE) 
	{
		pAd->VideoTurbine.Enable = GLOBAL_AP_VIDEO_CONFIG.Enable;
		pAd->VideoTurbine.ClassifierEnable = GLOBAL_AP_VIDEO_CONFIG.ClassifierEnable;
		pAd->VideoTurbine.HighTxMode = GLOBAL_AP_VIDEO_CONFIG.HighTxMode;
		pAd->VideoTurbine.TxPwr = GLOBAL_AP_VIDEO_CONFIG.TxPwr;
		pAd->VideoTurbine.VideoMCSEnable = GLOBAL_AP_VIDEO_CONFIG.VideoMCSEnable;
		pAd->VideoTurbine.VideoMCS = GLOBAL_AP_VIDEO_CONFIG.VideoMCS;
		pAd->VideoTurbine.TxBASize = GLOBAL_AP_VIDEO_CONFIG.TxBASize;
		pAd->VideoTurbine.TxLifeTimeMode = GLOBAL_AP_VIDEO_CONFIG.TxLifeTimeMode;
		pAd->VideoTurbine.TxLifeTime = GLOBAL_AP_VIDEO_CONFIG.TxLifeTime;
		pAd->VideoTurbine.TxRetryLimit = GLOBAL_AP_VIDEO_CONFIG.TxRetryLimit;
	}
}


VOID TxSwQDepthAdjust(IN RTMP_ADAPTER *pAd, IN UINT32 qLen)
{
	ULONG IrqFlags;
	INT qIdx;
	QUEUE_HEADER *pTxQ, *pEntry;
	PNDIS_PACKET pPacket;
	
	RTMP_IRQ_LOCK(&pAd->irq_lock, IrqFlags);
	pAd->TxSwQMaxLen = qLen;
	for (qIdx = 0; qIdx < NUM_OF_TX_RING; qIdx++)
	{
		pTxQ = &pAd->TxSwQueue[qIdx];
		while(pTxQ->Number >= pAd->TxSwQMaxLen)
		{
			pEntry = RemoveHeadQueue(pTxQ);
			if (pEntry)
			{
				pPacket = QUEUE_ENTRY_TO_PACKET(pEntry);
				RELEASE_NDIS_PACKET(pAd, pPacket, NDIS_STATUS_FAILURE);
			}
			else
				break;
		}
	}
	RTMP_IRQ_UNLOCK(&pAd->irq_lock, IrqFlags);
	
	DBGPRINT(RT_DEBUG_OFF, ("%s():Set TxSwQMaxLen as %d\n", 
			__FUNCTION__, pAd->TxSwQMaxLen));
}


VOID VideoTurbineDynamicTune(
	IN PRTMP_ADAPTER pAd)
{
	if (pAd->VideoTurbine.Enable == TRUE) 
	{
			UINT32 MacReg = 0;

		{
			/* Tx retry limit = 2F,1F */
			RTMP_IO_READ32(pAd, TX_RTY_CFG, &MacReg);
			MacReg &= 0xFFFF0000;
			MacReg |= GetAsicVideoRetry(pAd);
			RTMP_IO_WRITE32(pAd, TX_RTY_CFG, MacReg);
		}

		pAd->VideoTurbine.TxBASize = GetAsicVideoTxBA(pAd);

		Set_RateAdaptInterval(pAd, "100:50");
		TxSwQDepthAdjust(pAd, 1024);
			
	}
	else 
	{
			UINT32 MacReg = 0;


		/* Default Tx retry limit = 1F,0F */
		RTMP_IO_READ32(pAd, TX_RTY_CFG, &MacReg);
		MacReg &= 0xFFFF0000;
			MacReg |= GetAsicDefaultRetry(pAd);
		RTMP_IO_WRITE32(pAd, TX_RTY_CFG, MacReg);

		pAd->VideoTurbine.TxBASize = GetAsicDefaultTxBA(pAd);

		/* reset to default rate adaptation simping interval */
		if ((pAd->ra_interval != DEF_RA_TIME_INTRVAL) || 
			(pAd->ra_fast_interval != DEF_QUICK_RA_TIME_INTERVAL))
			Set_RateAdaptInterval(pAd, "500:100");

		TxSwQDepthAdjust(pAd, MAX_PACKETS_IN_QUEUE);
	}
}

UINT32 GetAsicDefaultRetry(
	IN PRTMP_ADAPTER pAd)
{
	UINT32 RetryLimit;

	RetryLimit = 0x1F0F;

	return RetryLimit;
}

UCHAR GetAsicDefaultTxBA(
	IN PRTMP_ADAPTER pAd)
{
        return pAd->CommonCfg.TxBASize;
}

UINT32 GetAsicVideoRetry(
	IN PRTMP_ADAPTER pAd)
{
	return pAd->VideoTurbine.TxRetryLimit;
}

UCHAR GetAsicVideoTxBA(
	IN PRTMP_ADAPTER pAd)
{
	return pAd->VideoTurbine.TxBASize;
}

VOID VideoConfigInit(
	IN PRTMP_ADAPTER pAd)
{
	pAd->VideoTurbine.Enable = FALSE;
	pAd->VideoTurbine.TxRetryLimit = 0x2F1F;
	pAd->VideoTurbine.TxBASize = pAd->CommonCfg.TxBASize; 
}

#endif /* VIDEO_TURBINE_SUPPORT */



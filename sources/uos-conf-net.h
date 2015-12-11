/*
 * uos-conf-net.h
 *  Created on: 11.12.2015
 *      Author: a_lityagin <alexraynepe196@gmail.com>
 *                         <alexraynepe196@hotbox.ru>
 *                         <alexraynepe196@mail.ru>
 *                         <alexrainpe196@hotbox.ru>
 *  
 *  *\~russian UTF8
 *  это дефолтовый конфигурационный файл уОС. здесь сведены настройки модулей 
 *  свзаные с сетевой системой.
 *  для сборки своей оси, скопируйте этот файл в папку своего проекта и 
 *      переопределите настройки.
 */

#ifndef UOS_CONF_NET_H_
#define UOS_CONF_NET_H_

/**************************************************************************
 *                              debug console
 ************************************************************************** */
//#define DEBUG_NET_ETH
//#define DEBUG_NET_ETH_FAIL
//#define ARP_TRACE
//#define DEBUG_NET_ARPTABLE
//#define DEBUG_NET_ROUTE
//#define DEBUG_NET_IP
//#define DEBUG_NET_ICMP

 /**************************************************************************
 *                              net
 ************************************************************************** */
//#   define ETH_STACKSZ        1000
//#   define ETH_INQ_SIZE       16
//#   define ETH_OUTQ_SIZE      8
//#   define ETH_MTU            1518    /* maximum ethernet frame length */

//#   define ETH_MDIO_KHZ       2500ul
//*   этот таймаут задает время устаревания PHY_STASTUS, после которого eth_phy_poll форсирует запрос статуса
//      с блокированием управляющей нитки 
//#   define ETH_PHY_STASTUS_TOus    10000ul


#define ETH_OPTIMISE_SPEED      1

//#define UDP_CHECKSUM_DISABLE

#endif /* UOS_CONF_NET_H_ */

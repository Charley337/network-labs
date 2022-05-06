#include "net.h"
#include "ip.h"
#include "ethernet.h"
#include "arp.h"
#include "icmp.h"

/**
 * @brief 处理一个收到的数据包
 * 
 * @param buf 要处理的数据包
 * @param src_mac 源mac地址
 */
void ip_in(buf_t *buf, uint8_t *src_mac)
{
    // TO-DO
    if(buf->len >= sizeof(ip_hdr_t)) {
        // check header
        ip_hdr_t *pkt = (ip_hdr_t*)buf->data;
        uint16_t total_len16 = swap16(pkt->total_len16);
        if(pkt->version == IP_VERSION_4 && total_len16 <= buf->len) {
            uint16_t hdr_checksum16 = pkt->hdr_checksum16;
            pkt->hdr_checksum16 = 0;
            if(checksum16((uint16_t*)pkt, pkt->hdr_len * 4) == hdr_checksum16) {
                pkt->hdr_checksum16 = hdr_checksum16;
                if(memcmp(pkt->dst_ip, net_if_ip, NET_IP_LEN) == 0) {
                    if(buf->len > total_len16) {
                        buf_remove_padding(buf, buf->len - total_len16);
                    }
                    buf_remove_header(buf, sizeof(ip_hdr_t));
                    if(net_in(buf, pkt->protocol, pkt->src_ip) != 0) {
                        icmp_unreachable(buf, pkt->src_ip, ICMP_CODE_PROTOCOL_UNREACH);
                    }
                }
            }
        }
    }
}

/**
 * @brief 处理一个要发送的ip分片
 * 
 * @param buf 要发送的分片
 * @param ip 目标ip地址
 * @param protocol 上层协议
 * @param id 数据包id
 * @param offset 分片offset，必须被8整除
 * @param mf 分片mf标志，是否有下一个分片
 */
void ip_fragment_out(buf_t *buf, uint8_t *ip, net_protocol_t protocol, int id, uint16_t offset, int mf)
{
    // TO-DO
    buf_add_header(buf, sizeof(ip_hdr_t));
    ip_hdr_t *pkt = (ip_hdr_t*)buf->data;
    pkt->version = IP_VERSION_4;
    pkt->hdr_len = sizeof(ip_hdr_t) / IP_HDR_LEN_PER_BYTE;
    pkt->tos = 0;   // 本实验中可将tos设为0
    pkt->total_len16 = swap16((uint16_t)(buf->len));
    pkt->id16 = swap16((uint16_t)id);
    pkt->flags_fragment16 = mf == 1 ? swap16(offset + IP_MORE_FRAGMENT) : swap16(offset);
    pkt->ttl = IP_DEFALUT_TTL;
    pkt->protocol = protocol;
    pkt->hdr_checksum16 = 0;
    memcpy(pkt->src_ip, net_if_ip, NET_IP_LEN);
    memcpy(pkt->dst_ip, ip, NET_IP_LEN);
    pkt->hdr_checksum16 = checksum16(pkt, sizeof(ip_hdr_t));    // 注意：只有首部校验和不需要大小端转换
    arp_out(buf, ip);
}

/**
 * @brief 处理一个要发送的ip数据包
 * 
 * @param buf 要处理的包
 * @param ip 目标ip地址
 * @param protocol 上层协议
 */
void ip_out(buf_t *buf, uint8_t *ip, net_protocol_t protocol)
{
    // TO-DO
    static int id = 0;
    if(buf->len > ETHERNET_MAX_TRANSPORT_UNIT - sizeof(ip_hdr_t)) {
        // need fragment
        size_t len = buf->len;
        uint16_t offset = 0;
        while(len > ETHERNET_MAX_TRANSPORT_UNIT - sizeof(ip_hdr_t)) {
            buf_t ip_buf;
            buf_init(&ip_buf, ETHERNET_MAX_TRANSPORT_UNIT - sizeof(ip_hdr_t));
            memcpy(ip_buf.data, buf->data + buf->len - len, ETHERNET_MAX_TRANSPORT_UNIT - sizeof(ip_hdr_t));
            ip_fragment_out(&ip_buf, ip, protocol, id, offset, 1);
            offset += (ETHERNET_MAX_TRANSPORT_UNIT - sizeof(ip_hdr_t)) / 8;
            len -= ETHERNET_MAX_TRANSPORT_UNIT - sizeof(ip_hdr_t);
        }
        buf_t ip_buf;
        buf_init(&ip_buf, len);
        memcpy(ip_buf.data, buf->data + buf->len - len, len);
        ip_fragment_out(&ip_buf, ip, protocol, id, offset, 0);
        id += 1;
    }
    else {
        ip_fragment_out(buf, ip, protocol, id, 0, 0);
        id += 1;
    }
}

/**
 * @brief 初始化ip协议
 * 
 */
void ip_init()
{
    net_add_protocol(NET_PROTOCOL_IP, ip_in);
}
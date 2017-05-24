/* Martin Vit support@voipmonitor.org
 * This program is free software, distributed under the terms of
 * the GNU General Public License Version 2.
*/

/* Calls are stored into indexed array. 
 * Into one calltable is stored SIP call-id and IP-port of SDP session
 */

#ifndef CALLTABLE_H
#define CALLTABLE_H

#include <queue>
#include <map>
#include <list>
#include <set>

#include <arpa/inet.h>
#include <time.h>
#include <limits.h>

#include <pcap.h>

#include <string>

#include "rtp.h"
#include "tools.h"
#include "sql_db.h"
#include "voipmonitor.h"
#include "tools_fifo_buffer.h"

#define MAX_IP_PER_CALL 40	//!< total maxumum of SDP sessions for one call-id
#define MAX_SSRC_PER_CALL 40	//!< total maxumum of SDP sessions for one call-id
#define MAX_FNAME 256		//!< max len of stored call-id
#define MAX_RTPMAP 40          //!< max rtpmap records
#define MAXNODE 150000
#define MAXLEN_SDP_SESSID 16
#define MAXLEN_SDP_TO 128
#define MAX_SIPCALLERDIP 4

#define INVITE 1
#define BYE 2
#define CANCEL 3
#define RES10X 100
#define RES18X 180
#define RES182 182
#define RES2XX 200
#define RES300 300
#define RES3XX 399
#define RES401 401
#define RES403 403
#define RES404 404
#define RES4XX 400
#define RES5XX 500
#define RES6XX 600
#define REGISTER 4
#define MESSAGE 5
#define INFO 6
#define SUBSCRIBE 7
#define OPTIONS 8
#define NOTIFY 9
#define ACK 10
#define PRACK 11
#define PUBLISH 12
#define REFER 13
#define UPDATE 14
#define SKINNY_NEW 100
#define SS7 200

#define IS_SIP_RES18X(sip_method) (sip_method == RES18X || sip_method == RES182)
#define IS_SIP_RES3XX(sip_method) (sip_method == RES300 || sip_method == RES3XX)
#define IS_SIP_RES4XX(sip_method) (sip_method == RES401 || sip_method == RES403 || sip_method == RES404 || sip_method == RES4XX)
#define IS_SIP_RESXXX(sip_method) (sip_method == RES10X || sip_method == RES18X || sip_method == RES182 || sip_method == RES2XX || IS_SIP_RES3XX(sip_method) || IS_SIP_RES4XX(sip_method) || sip_method == RES5XX || sip_method == RES6XX)

#define FLAG_SAVERTP		(1 << 0)
#define FLAG_SAVERTCP		(1 << 1)
#define FLAG_SAVESIP		(1 << 2)
#define FLAG_SAVEREGISTER	(1 << 3)
#define FLAG_SAVEAUDIO		(1 << 4)
#define FLAG_FORMATAUDIO_WAV	(1 << 5)
#define FLAG_FORMATAUDIO_OGG	(1 << 6)
#define FLAG_SAVEAUDIO_WAV	(FLAG_SAVEAUDIO|FLAG_FORMATAUDIO_WAV)
#define FLAG_SAVEAUDIO_OGG	(FLAG_SAVEAUDIO|FLAG_FORMATAUDIO_OGG)
#define FLAG_SAVEGRAPH		(1 << 7)
#define FLAG_SAVERTPHEADER	(1 << 8)
#define FLAG_SKIPCDR		(1 << 9)
#define FLAG_RUNSCRIPT		(1 << 10)
#define FLAG_RUNAMOSLQO		(1 << 11)
#define FLAG_RUNBMOSLQO		(1 << 12)
#define FLAG_HIDEMESSAGE	(1 << 13)
#define FLAG_USE_SPOOL_2	(1 << 14)

#define CHAN_SIP	1
#define CHAN_SKINNY	2

#define CDR_NEXT_MAX 10

#define CDR_CHANGE_SRC_PORT_CALLER	(1 << 0)
#define CDR_CHANGE_SRC_PORT_CALLED	(1 << 1)
#define CDR_UNCONFIRMED_BYE		(1 << 2)
#define CDR_ALONE_UNCONFIRMED_BYE	(1 << 3)

#define SS7_IAM 1
#define SS7_ACM 6
#define SS7_CPG 44
#define SS7_ANM 9
#define SS7_REL 12
#define SS7_RLC 16


struct s_dtmf {
	enum e_type {
		sip_info,
		inband,
		rfc2833
	};
	e_type type;
	double ts;
	char dtmf;
	unsigned int saddr;
	unsigned int daddr;
};

struct s_sdp_flags {
	s_sdp_flags() {
		is_fax = 0;
		rtcp_mux = 0;
	}
	int operator != (const s_sdp_flags &other) {
		return(is_fax != other.is_fax ||
		       rtcp_mux != other.rtcp_mux);
	}
	int16_t is_fax;
	int16_t rtcp_mux;
};

struct hash_node_call {
	hash_node_call *next;
	Call *call;
	int iscaller;
	u_int16_t is_rtcp;
	s_sdp_flags sdp_flags;
};

struct hash_node {
	hash_node *next;
	hash_node_call *calls;
	u_int32_t addr;
	u_int16_t port;
};

struct ip_port_call_info_rtp {
	volatile u_int32_t saddr;
	volatile u_int16_t sport;
	volatile u_int32_t daddr;
	volatile u_int16_t dport;
	volatile time_t last_packet_time;
};

struct ip_port_call_info {
	u_int32_t addr;
	u_int16_t port;
	bool iscaller;
	char sessid[MAXLEN_SDP_SESSID];
	char to[MAXLEN_SDP_TO];
	u_int32_t sip_src_addr;
	s_sdp_flags sdp_flags;
	ip_port_call_info_rtp rtp[2];
};

struct raws_t {
	int ssrc_index;
	int rawiterator;
	int codec;
	struct timeval tv;
	string filename;
};

class Call_abstract {
public:
	Call_abstract(int call_type, time_t time);
	int calltime() { return first_packet_time; };
	string get_sensordir();
	string get_pathname(eTypeSpoolFile typeSpoolFile, const char *substSpoolDir = NULL);
	string get_filename(eTypeSpoolFile typeSpoolFile, const char *fileExtension = NULL);
	string get_pathfilename(eTypeSpoolFile typeSpoolFile, const char *fileExtension = NULL);
	string dirnamesqlfiles();
	char *get_fbasename_safe();
	const char *getSpoolDir(eTypeSpoolFile typeSpoolFile) {
		return(::getSpoolDir(typeSpoolFile, getSpoolIndex()));
	}
	int getSpoolIndex() {
		extern sExistsColumns existsColumns;
		return((flags & FLAG_USE_SPOOL_2) && isSetSpoolDir2() &&
			((type == INVITE && existsColumns.cdr_next_spool_index) ||
			 (type == MESSAGE && existsColumns.message_spool_index)) ?
			1 : 
			0);
	}
	bool isEmptyChunkBuffersCount() {
		return(!chunkBuffersCount);
	}
	void incChunkBuffers() {
		__sync_add_and_fetch(&chunkBuffersCount, 1);
	}
	void decChunkBuffers() {
		__sync_sub_and_fetch(&chunkBuffersCount, 1);
	}
	void addTarPos(u_int64_t pos, int type);
public:
	int type;
	time_t first_packet_time;
	char fbasename[MAX_FNAME];
	char fbasename_safe[MAX_FNAME];
	u_int64_t fname_register;
	int useSensorId;
	int useDlt;
	pcap_t *useHandle;
	string force_spool_path;
	volatile unsigned int flags;
protected:
	list<u_int64_t> tarPosSip;
	list<u_int64_t> tarPosRtp;
	list<u_int64_t> tarPosGraph;
private:
	volatile u_int16_t chunkBuffersCount;
};

/**
  * This class implements operations on call
*/
class Call : public Call_abstract {
public:
	struct sSipcalleRD_IP {
		sSipcalleRD_IP() {
			for(unsigned i = 0; i < MAX_SIPCALLERDIP; i++) {
				sipcallerip[i] = 0;
				sipcalledip[i] = 0;
			}
		}
		u_int32_t sipcallerip[MAX_SIPCALLERDIP];
		u_int32_t sipcalledip[MAX_SIPCALLERDIP];
	};
	struct sMergeLegInfo {
		sMergeLegInfo() {
			seenbye = false;
			seenbye_time_usec = 0;
		}
		bool seenbye;
		u_int64_t seenbye_time_usec;
	};
	struct sInviteSD_Addr {
		sInviteSD_Addr() {
			confirmed = false;
		}
		u_int32_t saddr;
		u_int32_t daddr;
		u_int16_t sport;
		u_int16_t dport;
		bool confirmed;
	};
	struct sSipResponse {
		sSipResponse(const char *SIPresponse = NULL, int SIPresponseNum = 0) {
			if(SIPresponse) {
				this->SIPresponse = SIPresponse;
			}
			this->SIPresponseNum = SIPresponseNum;
		}
		string SIPresponse;
		int SIPresponseNum;
	};
	struct sSipHistory {
		sSipHistory(u_int64_t time = 0,
			    const char *SIPrequest = NULL,
			    const char *SIPresponse = NULL, int SIPresponseNum = 0) {
			this->time = time;
			if(SIPrequest && SIPrequest[0]) {
				this->SIPrequest = SIPrequest;
			}
			if(SIPresponse && SIPresponse[0]) {
				this->SIPresponse = SIPresponse;
			}
			this->SIPresponseNum = SIPresponseNum;
		}
		u_int64_t time;
		string SIPrequest;
		string SIPresponse;
		int SIPresponseNum;
	};
	struct sRtcpXrDataItem {
		timeval tv;
		int16_t moslq;
		int16_t nlr;
	};
	struct sRtcpXrDataSsrc : public list<sRtcpXrDataItem> {
		void add(timeval tv, int16_t moslq, int16_t nlr) {
			sRtcpXrDataItem dataItem;
			dataItem.tv = tv;
			dataItem.moslq = moslq;
			dataItem.nlr = nlr;
			this->push_back(dataItem);
		}
	};
	struct sRtcpXrData : public map<u_int32_t, sRtcpXrDataSsrc> {
		void add(u_int32_t ssrc, timeval tv, int16_t moslq, int16_t nlr) {
			(*this)[ssrc].add(tv, moslq, nlr);
		}
	};
	struct sUdptlDumper {
		sUdptlDumper() {
			dumper = NULL;
			last_seq = 0;
		}
		~sUdptlDumper() {
			if(dumper) {
				delete dumper;
			}
		}
		PcapDumper *dumper;
		unsigned last_seq;
	};
	enum eVoicemail {
		voicemail_na,
		voicemail_active,
		voicemail_inactive
	};
public:
	bool is_ssl;			//!< call was decrypted
	char chantype;
	RTP *rtp[MAX_SSRC_PER_CALL];		//!< array of RTP streams
	volatile int rtplock;
	unsigned long call_id_len;	//!< length of call-id 	
	string call_id;	//!< call-id from SIP session
	char callername[256];		//!< callerid name from SIP header
	char caller[256];		//!< From: xxx 
	char caller_domain[256];	//!< From: xxx 
	char called[256];		//!< To: xxx
	map<string, string> called_invite_branch_map;
	char called_domain[256];	//!< To: xxx
	char contact_num[64];		//!< 
	char contact_domain[128];	//!< 
	char digest_username[64];	//!< 
	char digest_realm[64];		//!< 
	int register_expires;	
	char byecseq[32];		
	char invitecseq[32];		
	char cancelcseq[32];		
	char updatecseq[32];		
	char custom_header1[256];	//!< Custom SIP header
	char match_header[128];	//!< Custom SIP header
	bool seeninvite;		//!< true if we see SIP INVITE within the Call
	bool seeninviteok;			//!< true if we see SIP INVITE within the Call
	bool seenbye;			//!< true if we see SIP BYE within the Call
	u_int64_t seenbye_time_usec;
	bool seenbyeandok;		//!< true if we see SIP OK TO BYE OR TO CANEL within the Call
	u_int64_t seenbyeandok_time_usec;
	bool unconfirmed_bye;
	bool seenRES2XX;
	bool seenRES2XX_no_BYE;
	bool seenRES18X;
	bool sighup;			//!< true if call is saving during sighup
	char a_ua[1024];		//!< caller user agent 
	char b_ua[1024];		//!< callee user agent 
	int rtpmap[MAX_IP_PER_CALL][MAX_RTPMAP]; //!< rtpmap for every rtp stream
	RTP tmprtp;			//!< temporary structure used to decode information from frame
	RTP *lastcallerrtp;		//!< last RTP stream from caller
	RTP *lastcalledrtp;		//!< last RTP stream from called
	u_int32_t saddr;		//!< source IP address of first INVITE
	unsigned short sport;		//!< source port of first INVITE
	int whohanged;			//!< who hanged up. 0 -> caller, 1-> callee, -1 -> unknown
	int recordstopped;		//!< flag holding if call was stopped to avoid double free
	int dtmfflag;			//!< used for holding dtmf states 
	unsigned int dtmfflag2[2];	//!< used for holding dtmf states 
	double lastdtmf_time;		//!< used for holding time of last dtmf

	int silencerecording;
	int recordingpausedby182;
	int msgcount;
	int regcount;
	int reg401count;
	int reg401count_distinct;
	u_int32_t reg401count_sipcallerip[MAX_SIPCALLERDIP];
	int reg403count;
	int reg403count_distinct;
	u_int32_t reg403count_sipcallerip[MAX_SIPCALLERDIP];
	int reg200count;
	int regstate;
	bool regresponse;
	timeval regrrdstart;		// time of first REGISTER
	int regrrddiff;			// RRD diff time REGISTER<->OK in [ms]- RFC6076
	uint64_t regsrcmac;		// mac if ether layer present in REGISTER
	unsigned long long flags1;	//!< bit flags used to store max 64 flags 
	volatile unsigned int rtppacketsinqueue;
	volatile int end_call;
	volatile int push_call_to_calls_queue;
	volatile int push_register_to_registers_queue;
	unsigned int unrepliedinvite;
	unsigned int ps_drop;
	unsigned int ps_ifdrop;
	char forcemark[2];
	queue<u_int64_t> forcemark_time[2];
	volatile int _forcemark_lock;
	int first_codec;
	bool	has_second_merged_leg;

	float a_mos_lqo;
	float b_mos_lqo;

	time_t progress_time;		//!< time in seconds of 18X response
	time_t first_rtp_time;		//!< time in seconds of first RTP packet
	time_t connect_time;		//!< time in seconds of 200 OK
	unsigned int connect_time_usec;
	time_t last_packet_time;	
	time_t last_rtp_a_packet_time;	
	time_t last_rtp_b_packet_time;	
	unsigned int first_packet_usec;
	time_t destroy_call_at;
	time_t destroy_call_at_bye;
	time_t destroy_call_at_bye_confirmed;
	std::queue <s_dtmf> dtmf_history;
	
	u_int64_t first_invite_time_usec;
	u_int64_t first_response_100_time_usec;
	u_int64_t first_response_xxx_time_usec;
	u_int64_t first_message_time_usec;
	u_int64_t first_response_200_time_usec;

	uint8_t	caller_sipdscp;
	uint8_t	called_sipdscp;

	int isfax;
	char seenudptl;
	bool exists_udptl_data;
	bool not_acceptable;

	void *rtp_cur[2];		//!< last RTP structure in direction 0 and 1 (iscaller = 1)
	void *rtp_prev[2];		//!< previouse RTP structure in direction 0 and 1 (iscaller = 1)

	u_int32_t sipcallerip[MAX_SIPCALLERDIP];	//!< SIP signalling source IP address
	u_int32_t sipcalledip[MAX_SIPCALLERDIP];	//!< SIP signalling destination IP address
	map<string, sSipcalleRD_IP> map_sipcallerdip;
	u_int32_t lastsipcallerip;
	bool sipcallerdip_reverse;
	
	list<sInviteSD_Addr> invite_sdaddr;

	u_int16_t sipcallerport;
	u_int16_t sipcalledport;

	char lastSIPresponse[128];
	int lastSIPresponseNum;
	list<sSipResponse> SIPresponse;
	list<sSipHistory> SIPhistory;
	bool new_invite_after_lsr487;
	bool cancel_lsr487;
	
	int reason_sip_cause;
	string reason_sip_text;
	int reason_q850_cause;
	string reason_q850_text;

	char *contenttype;
	char *message;
	char *message_info;
	int content_length;
	
	unsigned int dcs;
	eVoicemail voicemail;

	int last_callercodec;		//!< Last caller codec 
	int last_calledcodec;		//!< Last called codec 

	int codec_caller;
	int codec_called;
	
	unsigned max_length_sip_data;
	unsigned max_length_sip_packet;

	FifoBuffer *audiobuffer1;
	int last_seq_audiobuffer1;
	u_int32_t last_ssrc_audiobuffer1;
	FifoBuffer *audiobuffer2;
	int last_seq_audiobuffer2;
	u_int32_t last_ssrc_audiobuffer2;

	unsigned int skinny_partyid;

	int *listening_worker_run;
	pthread_mutex_t listening_worker_run_lock;

	int thread_num;
	int thread_num_rd;

	char oneway;
	char absolute_timeout_exceeded;
	char zombie_timeout_exceeded;
	char bye_timeout_exceeded;
	char rtp_timeout_exceeded;
	char sipwithoutrtp_timeout_exceeded;
	char oneway_timeout_exceeded;
	char force_terminate;
	char pcap_drop;
	unsigned int lastsrcip;

	void *listening_worker_args;
	
	int ssrc_n;				//!< last index of rtp array
	int ipport_n;				//!< last index of addr and port array 

	RTP *lastraw[2];

	string geoposition;

	/* obsolete
	map<string, string> custom_headers;
	*/
	map<int, map<int, dstring> > custom_headers;

	volatile int _proxies_lock;
	list<unsigned int> proxies;
	
	bool onCall_2XX;
	bool onCall_18X;
	bool updateDstnumOnAnswer;
	bool updateDstnumFromMessage;
	
	bool force_close;

	unsigned int caller_silence;
	unsigned int called_silence;
	unsigned int caller_noise;
	unsigned int called_noise;
	unsigned int caller_lastsilence;
	unsigned int called_lastsilence;

	unsigned int caller_clipping_8k;
	unsigned int called_clipping_8k;
	
	int vlan;

	unsigned int lastcallerssrc;
	unsigned int lastcalledssrc;

	map<string, sMergeLegInfo> mergecalls;

	bool rtp_zeropackets_stored;
	
	sRtcpXrData rtcpXrData;
	
	unsigned last_udptl_seq;

	u_int32_t iscaller_consecutive[2];
	
	/**
	 * constructor
	 *
	 * @param call_id unique identification of call parsed from packet
	 * @param call_id_len lenght of the call_id buffer
	 * @param time time of the first packet
	 * 
	*/
	Call(int call_type, char *call_id, unsigned long call_id_len, time_t time);

	/**
	 * destructor
	 * 
	*/
	~Call();

	int get_index_by_ip_port(in_addr_t addr, unsigned short port);
	
	int get_index_by_sessid_to(char *sessid, char *to, in_addr_t sip_src_addr = 0);

	/**
	 * @brief close all rtp[].gfileRAW
	 *
	 * close all RTP[].gfileRAW to flush writes 
	 * 
	*/
	void closeRawFiles();
	
	/**
	 * @brief read RTP packet 
	 *
	 * Used for reading RTP packet 
	 * 
	*/
	bool read_rtp(struct packet_s *packetS, int iscaller, bool find_by_dest, bool stream_in_multiple_calls, char is_fax, char enable_save_packet, char *ifname = NULL);

	/**
	 * @brief read RTCP packet 
	 *
	 * Used for reading RTCP packet 
	 * 
	*/
	bool read_rtcp(struct packet_s *packetS, int iscaller, char enable_save_packet);

	/**
	 * @brief adds RTP stream to the this Call 
	 *
	 * Adds RTP stream to the this Call which is identified by IP address and port number
	 *
	 * @param addr IP address of the RTP stream
	 * @param port port number of the RTP stream
	 * 
	 * @return return 0 on success, 1 if IP and port is duplicated and -1 on failure
	*/
	int add_ip_port(in_addr_t sip_src_addr, in_addr_t addr, unsigned short port, pcap_pkthdr *header, char *sessid, char *to, bool iscaller, int *rtpmap, s_sdp_flags sdp_flags);
	
	bool refresh_data_ip_port(in_addr_t addr, unsigned short port, pcap_pkthdr *header, bool iscaller, int *rtpmap, s_sdp_flags sdp_flags);
	
	void add_ip_port_hash(in_addr_t sip_src_addr, in_addr_t addr, unsigned short port, pcap_pkthdr *header, char *sessid, char *to, bool iscaller, int *rtpmap, s_sdp_flags sdp_flags);

	/**
	 * @brief get pointer to PcapDumper of the writing pcap file  
	 *
	 * @return pointer to PcapDumper
	*/
	PcapDumper *getPcap() { return(&this->pcap); }
	PcapDumper *getPcapSip() { return(&this->pcapSip); }
	PcapDumper *getPcapRtp() { return(&this->pcapRtp); }
	
	/**
	 * @brief get time of the last seen packet which belongs to this call
	 *
	 * @return time of the last packet in seconds from UNIX epoch
	*/
	time_t get_last_packet_time() { return last_packet_time; };

	/**
	 * @brief get time of the last seen rtp packet which belongs to this call
	 *
	 * @return time of the last rtp packet in seconds from UNIX epoch
	*/
	time_t get_last_rtp_packet_time() { return max(last_rtp_a_packet_time, last_rtp_b_packet_time); };

	/**
	 * @brief set time of the last seen packet which belongs to this call
	 *
	 * this time is used for calculating lenght of the call
	 *
	*/
	void set_last_packet_time(time_t mtime) { if(mtime > last_packet_time) last_packet_time = mtime; };

	/**
	 * @brief get first time of the the packet which belongs to this call
	 *
	 * this time is used as start of the call in CDR record
	 *
	 * @return time of the first packet in seconds from UNIX epoch
	*/
	time_t get_first_packet_time() { return first_packet_time; };

	/**
	 * @brief set first time of the the packet which belongs to this call
	 *
	*/
	void set_first_packet_time(time_t mtime, unsigned int usec) { first_packet_time = mtime; first_packet_usec = usec;};

	/**
	 * @brief convert raw files to one WAV
	 *
	*/
	int convertRawToWav();
 
	/**
	 * @brief save call to database
	 *
	*/
	int saveToDb(bool enableBatchIfPossible = true);
	int saveAloneByeToDb(bool enableBatchIfPossible = true);

	/**
	 * @brief save register msgs to database
	 *
	*/
	int saveRegisterToDb(bool enableBatchIfPossible = true);

	/**
	 * @brief save sip MSSAGE to database
	 *
	*/
	int saveMessageToDb(bool enableBatchIfPossible = true);

	/**
	 * @brief calculate duration of the call
	 *
	 * @return lenght of the call in seconds
	*/
	int duration() { return last_packet_time - first_packet_time; };
	
	int connect_duration() { return(connect_time ? duration() - (connect_time - first_packet_time) : 0); };
	
	int duration_active() { return(getGlobalPacketTimeS() - first_packet_time); };
	
	int connect_duration_active() { return(connect_time ? duration_active() - (connect_time - first_packet_time) : 0); };
	
	/**
	 * @brief remove call from hash table
	 *
	*/
	void hashRemove();
	
	void skinnyTablesRemove();
	
	void removeFindTables(bool set_end_call = false);

	/**
	 * @brief remove all RTP 
	 *
	*/
	void removeRTP();

	/**
	 * @brief stop recording packets to pcap file
	 *
	*/
	void stoprecording();

	/**
	 * @brief save call to register tables and remove from calltable 
	 *
	*/
	void saveregister();

	/**
	 * @brief print debug information for the call to stdout
	 *
	*/

	void evProcessRtpStream(int index_ip_port, bool by_dest, u_int32_t saddr, u_int16_t sport, u_int32_t daddr, u_int16_t dport, time_t time) {
		if(index_ip_port < ipport_n) {
			if(!ip_port[index_ip_port].rtp[by_dest].saddr) {
				ip_port[index_ip_port].rtp[by_dest].saddr = saddr;
				ip_port[index_ip_port].rtp[by_dest].sport = sport;
				ip_port[index_ip_port].rtp[by_dest].daddr = daddr;
				ip_port[index_ip_port].rtp[by_dest].dport = dport;
				this->evStartRtpStream(index_ip_port, saddr, sport, daddr, dport, time);
			}
			ip_port[index_ip_port].rtp[by_dest].last_packet_time = time;
		}
	}
	void evDestroyIpPortRtpStream(int index_ip_port) {
		if(index_ip_port < ipport_n) {
			for(int i = 0; i < 2; i++) {
				if(ip_port[index_ip_port].rtp[i].saddr) {
					this->evEndRtpStream(index_ip_port, 
							     ip_port[index_ip_port].rtp[i].saddr,
							     ip_port[index_ip_port].rtp[i].sport,
							     ip_port[index_ip_port].rtp[i].daddr,
							     ip_port[index_ip_port].rtp[i].dport,
							     ip_port[index_ip_port].rtp[i].last_packet_time);
				}
			}
			this->nullIpPortInfoRtpStream(index_ip_port);
		}
	}
	void nullIpPortInfoRtpStream(int index_ip_port) {
		if(index_ip_port < ipport_n) {
			for(int i = 0; i < 2; i++) {
				ip_port[index_ip_port].rtp[i].saddr = 0;
				ip_port[index_ip_port].rtp[i].sport = 0;
				ip_port[index_ip_port].rtp[i].daddr = 0;
				ip_port[index_ip_port].rtp[i].dport = 0;
				ip_port[index_ip_port].rtp[i].last_packet_time = 0;
			}
		}
	}
	void evStartRtpStream(int index_ip_port, u_int32_t saddr, u_int16_t sport, u_int32_t daddr, u_int16_t dport, time_t time);
	void evEndRtpStream(int index_ip_port, u_int32_t saddr, u_int16_t sport, u_int32_t daddr, u_int16_t dport, time_t time);
	
	void addtocachequeue(string file);
	static void _addtocachequeue(string file);

	void addtofilesqueue(eTypeSpoolFile typeSpoolFile, string file, long long writeBytes);
	static void _addtofilesqueue(eTypeSpoolFile typeSpoolFile, string file, string dirnamesqlfiles, long long writeBytes, int spoolIndex);

	float mos_lqo(char *deg, int samplerate);

	void handle_dtmf(char dtmf, double dtmf_time, unsigned int saddr, unsigned int daddr, s_dtmf::e_type dtmf_type);
	
	void handle_dscp(struct iphdr2 *header_ip, bool iscaller);
	
	bool check_is_caller_called(const char *call_id, int sip_method, unsigned int saddr, unsigned int daddr, bool *iscaller, bool *iscalled = NULL, bool enableSetSipcallerdip = false);

	void dump();

	bool isFillRtpMap(int index) {
		for(int i = 0; i < MAX_RTPMAP; i++) {
			if(rtpmap[index][i]) {
				return(true);
			}
		}
		return(false);
	}

	int getFillRtpMapByCallerd(bool iscaller) {
		for(int i = ipport_n - 1; i >= 0; i--) {
			if(ip_port[i].iscaller == iscaller &&
			   isFillRtpMap(i)) {
				return(i);
			}
		}
		return(-1);
	}

	void atFinish();
	
	bool isPcapsClose() {
		return(pcap.isClose() &&
		       pcapSip.isClose() &&
		       pcapRtp.isClose());
	}
	bool isGraphsClose() {
		for(int i = 0; i < MAX_SSRC_PER_CALL; i++) {
			if(rtp[i] && !rtp[i]->graph.isClose()) {
				return(false);
			}
		}
		return(true);
	}
	bool isReadyForWriteCdr() {
		return(isPcapsClose() && isGraphsClose() &&
		       isEmptyChunkBuffersCount());
	}
	
	u_int32_t getAllReceivedRtpPackets();
	
	void forcemark_lock() {
		while(__sync_lock_test_and_set(&this->_forcemark_lock, 1));
	}
	void forcemark_unlock() {
		__sync_lock_release(&this->_forcemark_lock);
	}

	void proxies_lock() {
		while(__sync_lock_test_and_set(&this->_proxies_lock, 1));
	}
	void proxies_unlock() {
		__sync_lock_release(&this->_proxies_lock);
	}
	
	void shift_destroy_call_at(pcap_pkthdr *header, int lastSIPresponseNum = 0) {
		if(this->destroy_call_at > 0) {
			extern int opt_register_timeout;
			time_t new_destroy_call_at = 
				this->type == REGISTER ?
					header->ts.tv_sec + opt_register_timeout :
					(this->seenbyeandok ?
						header->ts.tv_sec + 5 :
					 this->seenbye ?
						header->ts.tv_sec + 60 :
						header->ts.tv_sec + (lastSIPresponseNum == 487 || this->lastSIPresponseNum == 487 ? 15 : 5));
			if(new_destroy_call_at > this->destroy_call_at) {
				this->destroy_call_at = new_destroy_call_at;
			}
		}
	}
	
	void applyRtcpXrDataToRtp();
	
	void adjustUA();
	
	void proxies_undup(set<unsigned int> *proxies_undup);

	void proxy_add(u_int32_t sipproxyip);
	
	void createListeningBuffers();
	void destroyListeningBuffers();
	void disableListeningBuffers();
	
	bool checkKnownIP_inSipCallerdIP(u_int32_t ip) {
		for(int i = 0; i < MAX_SIPCALLERDIP; i++) {
			if(ip == sipcallerip[i] ||
			   ip == sipcalledip[i]) {
				return(true);
			}
		}
		return(false);
	}
	
	u_int32_t getSipcalledipConfirmed(u_int16_t *dport);
	
	void calls_counter_inc() {
		extern volatile int calls_counter;
		if(type == INVITE || type == MESSAGE) {
			++calls_counter;
		}
	}
	void calls_counter_dec() {
		extern volatile int calls_counter;
		if(type == INVITE || type == MESSAGE) {
			--calls_counter;
		}
	}
	
	bool selectRtpStreams();
	bool selectRtpStreams_bySipcallerip();
	bool selectRtpStreams_byMaxLengthInLink();
	u_int64_t getFirstTimeInRtpStreams(int caller, bool selected);
	void printSelectedRtpStreams(int caller, bool selected);
	bool existsConcurenceInSelectedRtpStream(int caller, unsigned tolerance_ms);
	bool existsBothDirectionsInSelectedRtpStream();
	
	void setSipcallerip(u_int32_t ip, const char *call_id) {
		extern char opt_callidmerge_header[128];
		sipcallerip[0] = ip;
		if(opt_callidmerge_header[0] != '\0' &&
		   call_id && *call_id) {
			map_sipcallerdip[call_id].sipcallerip[0] = ip;
		}
	}
	void setSipcalledip(u_int32_t ip, const char *call_id) {
		extern char opt_callidmerge_header[128];
		sipcalledip[0] = ip;
		if(opt_callidmerge_header[0] != '\0' &&
		   call_id && *call_id) {
			map_sipcallerdip[call_id].sipcalledip[0] = ip;
		}
	}
	void setSeenbye(bool seenbye, u_int64_t seenbye_time_usec, const char *call_id) {
		extern char opt_callidmerge_header[128];
		this->seenbye = seenbye;
		this->seenbye_time_usec = seenbye_time_usec;
		if(opt_callidmerge_header[0] != '\0' &&
		   call_id && *call_id) {
			mergecalls[call_id].seenbye = seenbye;
			mergecalls[call_id].seenbye_time_usec = seenbye_time_usec;
		}
	}
	u_int64_t getSeenbyeTimeUS() {
		extern char opt_callidmerge_header[128];
		if(opt_callidmerge_header[0] != '\0') {
			u_int64_t seenbye_time_usec = 0;
			for(map<string, sMergeLegInfo>::iterator it = mergecalls.begin(); it != mergecalls.end(); ++it) {
				if(!it->second.seenbye || !it->second.seenbye_time_usec) {
					return(0);
				}
				if(seenbye_time_usec < it->second.seenbye_time_usec) {
					seenbye_time_usec = it->second.seenbye_time_usec;
				}
			}
			return(seenbye_time_usec);
		}
		return(seenbye ? seenbye_time_usec : 0);
	}

private:
	ip_port_call_info ip_port[MAX_IP_PER_CALL];
	PcapDumper pcap;
	PcapDumper pcapSip;
	PcapDumper pcapRtp;
	map<sStreamId, sUdptlDumper*> udptlDumpers;
public:
	bool error_negative_payload_length;
	bool use_removeRtp;
	volatile int hash_counter;
	bool use_rtcp_mux;
	bool use_sdp_sendonly;
	bool rtp_from_multiple_sensors;
	volatile int in_preprocess_queue_before_process_packet;
	volatile u_int32_t in_preprocess_queue_before_process_packet_at;
};

typedef struct {
	Call *call;
	int is_rtcp;
	s_sdp_flags sdp_flags;
	int iscaller;
} Ipportnode;


void adjustUA(char *ua);

inline unsigned int tuplehash(u_int32_t addr, u_int16_t port) {
	unsigned int key;

	key = (unsigned int)(addr * port);
	key += ~(key << 15);
	key ^=  (key >> 10);
	key +=  (key << 3);
	key ^=  (key >> 6);
	key += ~(key << 11);
	key ^=  (key >> 16);
	return key % MAXNODE;
}


class Ss7 : public Call_abstract {
public:
	enum eState {
		call_setup,
		in_call,
		completed,
		rejected,
		canceled
	};
	enum eMessageType {
		iam,
		acm,
		cpg,
		anm,
		rel,
		rlc
	};
	struct sParseData {
		sParseData() {
			isup_message_type = 0;
			isup_cic = 0;
			isup_satellite_indicator = 0;
			isup_echo_control_device_indicator = 0;
			isup_calling_partys_category = 0;
			isup_calling_party_nature_of_address_indicator = 0;
			isup_ni_indicator = 0;
			isup_address_presentation_restricted_indicator = 0;
			isup_screening_indicator = 0;
			isup_transmission_medium_requirement = 0;
			isup_called_party_nature_of_address_indicator = 0;
			isup_inn_indicator = 0;
			m3ua_protocol_data_opc = 0;
			m3ua_protocol_data_dpc = 0;
			mtp3_opc = 0;
			mtp3_dpc = 0;
			isup_cause_indicator = 0;
		}
		bool parse(struct packet_s_stack *packetS, const char *dissect_rslt = NULL);
		string ss7_id() {
			if(!isOk()) {
				return("");
			}
			unsigned opc = isset_unsigned(m3ua_protocol_data_opc) ? m3ua_protocol_data_opc : mtp3_opc;
			unsigned dpc = isset_unsigned(m3ua_protocol_data_dpc) ? m3ua_protocol_data_dpc : mtp3_dpc;
			unsigned low_point = min(opc, dpc);
			unsigned high_point = max(opc, dpc);
			return(intToString(isup_cic) + "-" + intToString(low_point) + "-" + intToString(high_point));
		}
		string filename() {
			if(!isOk()) {
				return("");
			}
			unsigned opc = isset_unsigned(m3ua_protocol_data_opc) ? m3ua_protocol_data_opc : mtp3_opc;
			unsigned dpc = isset_unsigned(m3ua_protocol_data_dpc) ? m3ua_protocol_data_dpc : mtp3_dpc;
			return(intToString(isup_cic) + "-" + intToString(opc) + "-" + intToString(dpc) + "-" +
			       e164_calling_party_number_digits + "-" + e164_called_party_number_digits);
		}
		bool isOk() {
			return(isset_unsigned(isup_cic) &&
			       (isset_unsigned(m3ua_protocol_data_opc) || isset_unsigned(mtp3_opc)) &&
			       (isset_unsigned(m3ua_protocol_data_dpc) || isset_unsigned(mtp3_dpc)));
		}
		bool isset_unsigned(unsigned value) {
			return(value != UINT_MAX);
		}
		void debugOutput();
		unsigned isup_message_type;
		unsigned isup_cic;
		unsigned isup_satellite_indicator;
		unsigned isup_echo_control_device_indicator;
		unsigned isup_calling_partys_category;
		unsigned isup_calling_party_nature_of_address_indicator;
		unsigned isup_ni_indicator;
		unsigned isup_address_presentation_restricted_indicator;
		unsigned isup_screening_indicator;
		unsigned isup_transmission_medium_requirement;
		unsigned isup_called_party_nature_of_address_indicator;
		unsigned isup_inn_indicator;
		unsigned m3ua_protocol_data_opc;
		unsigned m3ua_protocol_data_dpc;
		unsigned mtp3_opc;
		unsigned mtp3_dpc;
		string e164_called_party_number_digits;
		string e164_calling_party_number_digits;
		unsigned isup_cause_indicator;
	};
public:
	Ss7(time_t time);
	void processData(packet_s_stack *packetS, sParseData *data);
	void pushToQueue(string *ss7_id = NULL);
	int saveToDb(bool enableBatchIfPossible = true);
	string ss7_id() {
		return(iam_data.ss7_id());
	}
	string filename() {
		return(intToString(iam_time_us) + "-" + iam_data.filename());
	}
	eState getState() {
		return(rel_time_us ?
			(anm_time_us ? 
			  completed :
			(rel_cause_indicator == 16 ? 
			  canceled : 
			  rejected)) :
			(anm_time_us ? 
			  in_call : 
			  call_setup));
	}
	string getStateToString() {
		eState state = getState();
		return(state == call_setup ? "call_setup" :
		       state == in_call ? "in_call" :
		       state == completed ? "completed" :
		       state == rejected ? "rejected" :
		       state == canceled ? "canceled" : "");
	}
	string lastMessageTypeToString() {
		return(last_message_type == iam ? "iam" :
		       last_message_type == acm ? "acm" :
		       last_message_type == cpg ? "cpg" :
		       last_message_type == anm ? "anm" :
		       last_message_type == rel ? "rel" :
		       last_message_type == rlc ? "rlc" : "");
	}
	bool isset_unsigned(unsigned value) {
		return(value != UINT_MAX);
	}
private:
	void init();
public:
	eMessageType last_message_type;
	// IAM (Initial Address)
	sParseData iam_data;
	u_int32_t iam_src_ip;
	u_int32_t iam_dst_ip;
	u_int64_t iam_time_us;
	// ACM (Address Complete)
	u_int64_t acm_time_us;
	// CPG (Call Progress)
	u_int64_t cpg_time_us;
	// ANM (Answer)
	u_int64_t anm_time_us;
	// REL (Reelease)
	u_int64_t rel_time_us;
	// RLC (Release complete)
	u_int64_t rlc_time_us;
	u_int64_t last_time_us;
	unsigned rel_cause_indicator;
	PcapDumper pcap;
private:
	struct timeval last_dump_ts;
};


/**
  * This class implements operations on Call list
*/
class Calltable {
private:
	struct sAudioQueueThread {
		sAudioQueueThread() {
			thread_handle = 0;
			thread_id = 0;
		}
		pthread_t thread_handle;
		int thread_id;
	};
public:
	deque<Call*> calls_queue; //!< this queue is used for asynchronous storing CDR by the worker thread
	deque<Call*> audio_queue; //!< this queue is used for asynchronous audio convert by the worker thread
	deque<Call*> calls_deletequeue; //!< this queue is used for asynchronous storing CDR by the worker thread
	deque<Call*> registers_queue;
	deque<Call*> registers_deletequeue;
	deque<Ss7*> ss7_queue;
	queue<string> files_queue; //!< this queue is used for asynchronous storing CDR by the worker thread
	queue<string> files_sqlqueue; //!< this queue is used for asynchronous storing CDR by the worker thread
	map<string, Call*> calls_listMAP;
	map<string, Call*> calls_mergeMAP;
	map<string, Call*> registers_listMAP;
	map<string, Call*> skinny_ipTuples;
	map<unsigned int, Call*> skinny_partyID;
	map<string, Ss7*> ss7_listMAP;

	/**
	 * @brief constructor
	 *
	*/
	Calltable();
	/*
	Calltable() { 
		pthread_mutex_init(&qlock, NULL); 
		printf("SS:%d\n", sizeof(calls_hash));
		printf("SS:%s\n", 1);
		memset(calls_hash, 0x0, sizeof(calls_hash) * MAXNODE);
	};
	*/

	/**
	 * destructor
	 * 
	*/
	~Calltable();

	/**
	 * @brief lock calls_queue structure 
	 *
	*/
	void lock_calls_queue() { while(__sync_lock_test_and_set(&this->_sync_lock_calls_queue, 1)) usleep(10); /*pthread_mutex_lock(&qlock);*/ };
	void lock_calls_audioqueue() { while(__sync_lock_test_and_set(&this->_sync_lock_calls_audioqueue, 1)) usleep(10); /*pthread_mutex_lock(&qaudiolock);*/ };
	void lock_calls_deletequeue() { while(__sync_lock_test_and_set(&this->_sync_lock_calls_deletequeue, 1)) usleep(10); /*pthread_mutex_lock(&qdellock);*/ };
	void lock_registers_queue() { while(__sync_lock_test_and_set(&this->_sync_lock_registers_queue, 1)) usleep(10); };
	void lock_registers_deletequeue() { while(__sync_lock_test_and_set(&this->_sync_lock_registers_deletequeue, 1)) usleep(10); };
	void lock_files_queue() { while(__sync_lock_test_and_set(&this->_sync_lock_files_queue, 1)) usleep(10); /*pthread_mutex_lock(&flock);*/ };
	void lock_calls_listMAP() { while(__sync_lock_test_and_set(&this->_sync_lock_calls_listMAP, 1)) usleep(10); /*pthread_mutex_lock(&calls_listMAPlock);*/ };
	void lock_calls_mergeMAP() { while(__sync_lock_test_and_set(&this->_sync_lock_calls_mergeMAP, 1)) usleep(10); /*pthread_mutex_lock(&calls_mergeMAPlock);*/ };
	void lock_registers_listMAP() { while(__sync_lock_test_and_set(&this->_sync_lock_registers_listMAP, 1)) usleep(10); /*pthread_mutex_lock(&registers_listMAPlock);*/ };
	void lock_ss7_listMAP() { while(__sync_lock_test_and_set(&this->_sync_lock_ss7_listMAP, 1)) usleep(10); }
	void lock_process_ss7_listmap() { while(__sync_lock_test_and_set(&this->_sync_lock_process_ss7_listmap, 1)) usleep(10); }
	void lock_process_ss7_queue() { while(__sync_lock_test_and_set(&this->_sync_lock_process_ss7_queue, 1)) usleep(10); }

	/**
	 * @brief unlock calls_queue structure 
	 *
	*/
	void unlock_calls_queue() { __sync_lock_release(&this->_sync_lock_calls_queue); /*pthread_mutex_unlock(&qlock);*/ };
	void unlock_calls_audioqueue() { __sync_lock_release(&this->_sync_lock_calls_audioqueue); /*pthread_mutex_unlock(&qaudiolock);*/ };
	void unlock_calls_deletequeue() { __sync_lock_release(&this->_sync_lock_calls_deletequeue); /*pthread_mutex_unlock(&qdellock);*/ };
	void unlock_registers_queue() { __sync_lock_release(&this->_sync_lock_registers_queue); };
	void unlock_registers_deletequeue() { __sync_lock_release(&this->_sync_lock_registers_deletequeue); };
	void unlock_files_queue() { __sync_lock_release(&this->_sync_lock_files_queue); /*pthread_mutex_unlock(&flock);*/ };
	void unlock_calls_listMAP() { __sync_lock_release(&this->_sync_lock_calls_listMAP); /*pthread_mutex_unlock(&calls_listMAPlock);*/ };
	void unlock_calls_mergeMAP() { __sync_lock_release(&this->_sync_lock_calls_mergeMAP); /*pthread_mutex_unlock(&calls_mergeMAPlock);*/ };
	void unlock_registers_listMAP() { __sync_lock_release(&this->_sync_lock_registers_listMAP); /*pthread_mutex_unlock(&registers_listMAPlock);*/ };
	void unlock_ss7_listMAP() { __sync_lock_release(&this->_sync_lock_ss7_listMAP); };
	void unlock_process_ss7_listmap() { __sync_lock_release(&this->_sync_lock_process_ss7_listmap); };
	void unlock_process_ss7_queue() { __sync_lock_release(&this->_sync_lock_process_ss7_queue); };

	/**
	 * @brief add Call to Calltable
	 *
	 * @param call_id unique identifier of the Call which is parsed from the SIP packets
	 * @param call_id_len lenght of the call_id buffer
	 * @param time timestamp of arrivel packet in seconds from UNIX epoch
	 *
	 * @return reference of the new Call class
	*/
	Call *add(int call_type, char *call_id, unsigned long call_id_len, time_t time, u_int32_t saddr, unsigned short port, pcap_t *handle, int dlt, int sensorId);
	Ss7 *add_ss7(packet_s_stack *packetS, Ss7::sParseData *data);

	/**
	 * @brief find Call by call_id
	 *
	 * @param call_id unique identifier of the Call which is parsed from the SIP packets
	 * @param call_id_len lenght of the call_id buffer
	 *
	 * @return reference of the Call if found, otherwise return NULL
	*/
	Call *find_by_call_id(char *call_id, unsigned long call_id_len, time_t time) {
		Call *rslt_call = NULL;
		string call_idS = call_id_len ? string(call_id, call_id_len) : string(call_id);
		lock_calls_listMAP();
		map<string, Call*>::iterator callMAPIT = calls_listMAP.find(call_idS);
		if(callMAPIT != calls_listMAP.end() &&
		   !callMAPIT->second->end_call) {
			rslt_call = callMAPIT->second;
			if(time) {
				__sync_add_and_fetch(&rslt_call->in_preprocess_queue_before_process_packet, 1);
				rslt_call->in_preprocess_queue_before_process_packet_at = time;
			}
		}
		unlock_calls_listMAP();
		return(rslt_call);
	}
	Call *find_by_mergecall_id(char *call_id, unsigned long call_id_len, time_t time) {
		Call *rslt_call = NULL;
		string call_idS = call_id_len ? string(call_id, call_id_len) : string(call_id);
		lock_calls_mergeMAP();
		map<string, Call*>::iterator mergeMAPIT = calls_mergeMAP.find(call_idS);
		if(mergeMAPIT != calls_mergeMAP.end() &&
		   !mergeMAPIT->second->end_call) {
			rslt_call = mergeMAPIT->second;
			if(time) {
				__sync_add_and_fetch(&rslt_call->in_preprocess_queue_before_process_packet, 1);
				rslt_call->in_preprocess_queue_before_process_packet_at = time;
			}
		}
		unlock_calls_mergeMAP();
		return(rslt_call);
	}
	Call *find_by_register_id(char *register_id, unsigned long register_id_len) {
		Call *rslt_register = NULL;
		string register_idS = register_id_len ? string(register_id, register_id_len) : string(register_id);
		lock_registers_listMAP();
		map<string, Call*>::iterator registerMAPIT = registers_listMAP.find(register_idS);
		if(registerMAPIT != registers_listMAP.end() &&
		   !registerMAPIT->second->end_call) {
			rslt_register = registerMAPIT->second;
		}
		unlock_registers_listMAP();
		return(rslt_register);
	}
	Call *find_by_reference(long long callreference, bool lock) {
		Call *rslt_call = NULL;
		if(lock) lock_calls_listMAP();
		for(map<string, Call*>::iterator iter = calls_listMAP.begin(); iter != calls_listMAP.end(); iter++) {
			if((long long)(iter->second) == callreference) {
				rslt_call = iter->second;
				break;
			}
		}
		if(lock) unlock_calls_listMAP();
		return(rslt_call);
	}
	Call *find_by_skinny_partyid(unsigned int partyid);
	Call *find_by_skinny_ipTuples(unsigned int saddr, unsigned int daddr);
	Ss7 *find_by_ss7_id(string *ss7_id) {
		Ss7 *rslt_ss7 = NULL;
		lock_ss7_listMAP();
		map<string, Ss7*>::iterator ss7MAPIT = ss7_listMAP.find(*ss7_id);
		if(ss7MAPIT != ss7_listMAP.end()) {
			rslt_ss7 = ss7MAPIT->second;
		}
		unlock_ss7_listMAP();
		return(rslt_ss7);
	}

	/**
	 * @brief find Call by IP adress and port number
	 *
	 * @param addr IP address of the packet
	 * @param port port number of the packet
	 *
	 * @return reference of the Call if found, otherwise return NULL
	*/
	Call *find_by_ip_port(in_addr_t addr, unsigned short port, int *iscaller);

	/**
	 * @brief Save inactive calls to MySQL and delete it from list
	 *
	 *
	 * walk this list of Calls and if any of the call is inactive more
	 * than 5 minutes, save it to MySQL and delete it from the list
	 *
	 * @param currtime current time
	 *
	 * @return reference of the Call if found, otherwise return NULL
	*/
	int cleanup_calls( time_t currtime );
	int cleanup_registers( time_t currtime );
	int cleanup_ss7( time_t currtime );

	/**
	 * @brief add call to hash table
	 *
	*/
	void hashAdd(in_addr_t addr, unsigned short port, long int time_s, Call* call, int iscaller, int isrtcp, s_sdp_flags sdp_flags);


	/**
	 * @brief find call
	 *
	*/
	inline hash_node_call *hashfind_by_ip_port(in_addr_t addr, unsigned short port, bool lock = true) {
		hash_node *node = NULL;
		u_int32_t h = tuplehash(addr, port);
		if(lock) {
			lock_calls_hash();
		}
		hash_node_call *rslt = NULL;
		for (node = (hash_node *)calls_hash[h]; node != NULL; node = node->next) {
			if ((node->addr == addr) && (node->port == port)) {
				rslt = node->calls;
			}
		}
		if(lock) {
			unlock_calls_hash();
		}
		return rslt;
	}
	inline bool check_call_in_hashfind_by_ip_port(Call *call, in_addr_t addr, unsigned short port, bool lock = true) {
		bool rslt = false;
		if(lock) {
			lock_calls_hash();
		}
		hash_node_call *calls = this->hashfind_by_ip_port(addr, port, false);
		if(calls) {
			for(hash_node_call *node_call = (hash_node_call *)calls; node_call != NULL; node_call = node_call->next) {
				if(node_call->call == call) {
					rslt = true;
					break;
				}
			}
		}
		if(lock) {
			unlock_calls_hash();
		}
		return rslt;
	}

	/**
	 * @brief remove call from hash
	 *
	*/
	void hashRemove(Call *call, in_addr_t addr, unsigned short port, bool rtcp = false);
	int hashRemove(Call *call);
	
	void processCallsInAudioQueue(bool lock = true);
	static void *processAudioQueueThread(void *);
	size_t getCountAudioQueueThreads() {
		return(audioQueueThreads.size());
	}
	void setAudioQueueTerminating() {
		audioQueueTerminating = 1;
	}

	void destroyCallsIfPcapsClosed();
	void destroyRegistersIfPcapsClosed();
	
	void lock_calls_hash() {
		unsigned usleepCounter = 0;
		while(__sync_lock_test_and_set(&this->_sync_lock_calls_hash, 1)) {
			usleep(10 *
			       (usleepCounter > 10 ? 50 :
				usleepCounter > 5 ? 10 :
				usleepCounter > 2 ? 5 : 1));
			++usleepCounter;
		}
	}
	void unlock_calls_hash() {
		__sync_lock_release(&this->_sync_lock_calls_hash);
	}
private:
	/*
	pthread_mutex_t qlock;		//!< mutex locking calls_queue
	pthread_mutex_t qaudiolock;	//!< mutex locking calls_audioqueue
	pthread_mutex_t qdellock;	//!< mutex locking calls_deletequeue
	pthread_mutex_t flock;		//!< mutex locking calls_queue
	pthread_mutex_t calls_listMAPlock;
	pthread_mutex_t calls_mergeMAPlock;
	pthread_mutex_t registers_listMAPlock;
	*/

	void *calls_hash[MAXNODE];
	volatile int _sync_lock_calls_hash;
	volatile int _sync_lock_calls_listMAP;
	volatile int _sync_lock_calls_mergeMAP;
	volatile int _sync_lock_registers_listMAP;
	volatile int _sync_lock_calls_queue;
	volatile int _sync_lock_calls_audioqueue;
	volatile int _sync_lock_calls_deletequeue;
	volatile int _sync_lock_registers_queue;
	volatile int _sync_lock_registers_deletequeue;
	volatile int _sync_lock_files_queue;
	volatile int _sync_lock_ss7_listMAP;
	volatile int _sync_lock_process_ss7_listmap;
	volatile int _sync_lock_process_ss7_queue;
	
	list<sAudioQueueThread*> audioQueueThreads;
	unsigned int audioQueueThreadsMax;
	int audioQueueTerminating;
};


class CustomHeaders {
public:
	enum eType {
		cdr,
		message
	};
	enum eSpecialType {
		st_na,
		max_length_sip_data,
		max_length_sip_packet,
		gsm_dcs,
		gsm_voicemail
	};
	struct sCustomHeaderData {
		eSpecialType specialType;
		string header;
		unsigned db_id;
		string leftBorder;
		string rightBorder;
		string regularExpression;
		bool screenPopupField;
	};
	struct sCustomHeaderDataPlus : public sCustomHeaderData {
		string type;
		int dynamic_table;
		int dynamic_column;
	};
public:
	CustomHeaders(eType type);
	void load(class SqlDb *sqlDb = NULL, bool lock = true);
	void clear(bool lock = true);
	void refresh(SqlDb *sqlDb = NULL);
	void addToStdParse(ParsePacket *parsePacket);
	void parse(Call *call, char *data, int datalen, ParsePacket::ppContentsX *parseContents);
	void setCustomHeaderContent(Call *call, int pos1, int pos2, dstring *content, bool useLastValue = false);
	void prepareSaveRows_cdr(Call *call, class SqlDb_row *cdr_next, class SqlDb_row cdr_next_ch[], char *cdr_next_ch_name[]);
	void prepareSaveRows_message(Call *call, class SqlDb_row *message, class SqlDb_row message_next_ch[], char *message_next_ch_name[]);
	string getScreenPopupFieldsString(Call *call);
	string getDeleteQuery(const char *id, const char *prefix, const char *suffix);
	list<string> getAllNextTables() {
		return(allNextTables);
	}
	list<string> *getAllNextTablesPointer() {
		return(&allNextTables);
	}
	void createMysqlPartitions(class SqlDb *sqlDb);
	unsigned long getLoadTime() {
		return(loadTime);
	}
	string getQueryForSaveUseInfo(Call *call);
	void createTablesIfNotExists(SqlDb *sqlDb = NULL);
	void createTableIfNotExists(const char *tableName, SqlDb *sqlDb = NULL);
	void createColumnsForFixedHeaders(SqlDb *sqlDb = NULL);
	bool getPosForDbId(unsigned db_id, d_u_int32_t *pos);
private:
	void lock_custom_headers() {
		while(__sync_lock_test_and_set(&this->_sync_custom_headers, 1));
	}
	void unlock_custom_headers() {
		__sync_lock_release(&this->_sync_custom_headers);
	}
private:
	eType type;
	string configTable;
	string nextTablePrefix;
	string fixedTable;
	map<int, map<int, sCustomHeaderData> > custom_headers;
	list<string> allNextTables;
	unsigned loadTime;
	unsigned lastTimeSaveUseInfo;
	volatile int _sync_custom_headers;
};


class NoHashMessageRule {
public:
	NoHashMessageRule();
	~NoHashMessageRule();
	bool checkNoHash(Call *call);
	void load(const char *name, 
		  unsigned customHeader_db_id, const char *customHeader, 
		  const char *header_regexp, const char *content_regexp);
	void clean_list_regexp();
private:
	string name;
	unsigned customHeader_db_id;
	string customHeader_name;
	d_u_int32_t customHeader_pos;
	bool customHeader_ok;
	list<cRegExp*> header_regexp;
	list<cRegExp*> content_regexp;
};

class NoHashMessageRules {
public:
	NoHashMessageRules();
	~NoHashMessageRules();
	bool checkNoHash(Call *call);
	void load(class SqlDb *sqlDb = NULL, bool lock = true);
	void clear(bool lock = true);
	void refresh(SqlDb *sqlDb = NULL);
private:
	void lock_no_hash() {
		while(__sync_lock_test_and_set(&this->_sync_no_hash, 1));
	}
	void unlock_no_hash() {
		__sync_lock_release(&this->_sync_no_hash);
	}
private:
	list<NoHashMessageRule*> rules;
	unsigned int loadTime;
	volatile int _sync_no_hash;
};


int sip_request_name_to_int(const char *requestName, bool withResponse = false);
const char *sip_request_int_to_name(int requestCode, bool withResponse = false);


#endif

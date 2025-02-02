#ifndef FILTER_MYSQL_H
#define FILTER_MYSQL_H


#include <cstdlib>
#include <iostream>
#include <string>
#include <cmath>
#include <map>
#include <vector>
#include <deque>
#include <deque>

#include "calltable.h"

#define FLAG_RTP_ALL		(1UL << 0)
#define FLAG_RTP_HEAD		(1UL << 1)
#define FLAG_NORTP      	(1UL << 2)
#define FLAG_RTCP		(1UL << 3)
#define FLAG_NORTCP     	(1UL << 4)
#define FLAG_SIP		(1UL << 5)
#define FLAG_NOSIP      	(1UL << 6)
#define FLAG_REGISTER		(1UL << 7)
#define FLAG_NOREGISTER		(1UL << 8)
#define FLAG_GRAPH		(1UL << 9)
#define FLAG_NOGRAPH    	(1UL << 10)
#define FLAG_AUDIO		(1UL << 11)
#define FLAG_AUDIO_WAV		(1UL << 12)
#define FLAG_AUDIO_OGG		(1UL << 13)
#define FLAG_NOWAV      	(1UL << 14)
#define FLAG_SKIP       	(1UL << 15)
#define FLAG_NOSKIP     	(1UL << 16)
#define FLAG_SCRIPT     	(1UL << 17)
#define FLAG_NOSCRIPT   	(1UL << 18)
#define FLAG_AMOSLQO    	(1UL << 19)
#define FLAG_BMOSLQO    	(1UL << 20)
#define FLAG_ABMOSLQO   	(1UL << 21)
#define FLAG_NOMOSLQO   	(1UL << 22)
#define FLAG_HIDEMSG		(1UL << 23)
#define FLAG_SHOWMSG		(1UL << 24)
#define FLAG_SPOOL_2_SET	(1UL << 25)
#define FLAG_SPOOL_2_UNSET	(1UL << 26)
#define FLAG_DTMF_DB		(1UL << 27)
#define FLAG_NODTMF_DB		(1UL << 28)
#define FLAG_DTMF_PCAP		(1UL << 29)
#define FLAG_NODTMF_PCAP	(1UL << 30)
#define FLAG_OPTIONS_DB		(1UL << 31)
#define FLAG_NOOPTIONS_DB	(1UL << 32)
#define FLAG_OPTIONS_PCAP	(1UL << 33)
#define FLAG_NOOPTIONS_PCAP	(1UL << 34)
#define FLAG_NOTIFY_DB		(1UL << 35)
#define FLAG_NONOTIFY_DB	(1UL << 36)
#define FLAG_NOTIFY_PCAP	(1UL << 37)
#define FLAG_NONOTIFY_PCAP	(1UL << 38)
#define FLAG_SUBSCRIBE_DB	(1UL << 39)
#define FLAG_NOSUBSCRIBE_DB	(1UL << 40)
#define FLAG_SUBSCRIBE_PCAP	(1UL << 41)
#define FLAG_NOSUBSCRIBE_PCAP	(1UL << 42)

#define MAX_PREFIX 64

struct filter_db_row_base {
	filter_db_row_base() {
		direction = 0;
		rtp = 0;
		rtcp = 0;
		sip = 0;
		reg = 0;
		graph = 0;
		wav = 0;
		skip = 0;
		mos_lqo = 0;
		script = 0;
		hide_message = 0;
		spool_2 = 0;
		dtmf = 0;
		options = 0;
		notify = 0;
		subscribe = 0;
	}
	int direction;
	int rtp;
	int rtcp;
	int sip;
	int reg;
	int graph;
	int wav;
	int skip;
	int mos_lqo;
	int script;
	int hide_message;
	int spool_2;
	int dtmf;
	int options;
	int notify;
	int subscribe;
};

class filter_base {
protected:
	void loadBaseDataRow(class SqlDb_row *sqlRow, filter_db_row_base *baseRow);
	void loadBaseDataRow(map<string, string> *row, filter_db_row_base *baseRow);
	unsigned long int getFlagsFromBaseData(filter_db_row_base *baseRow);
	void setCallFlagsFromFilterFlags(volatile unsigned long int *callFlags, unsigned long int filterFlags);
};

class IPfilter : public filter_base {
private:
	struct db_row : filter_db_row_base {
		db_row() {
			ip.clear();
			mask = 0;
		}
		vmIP ip;
		int mask;
	};
        struct t_node {
		vmIP network;
		int mask;
		int direction;
		unsigned long int flags;
                t_node *next;
        };
        t_node *first_node;
public: 
        IPfilter();
        ~IPfilter();
        void load(SqlDb *sqlDb = NULL);
	int _add_call_flags(volatile unsigned long int *flags, vmIP saddr, vmIP daddr);
        static void dump2man(ostringstream &oss);
	static int add_call_flags(volatile unsigned long int *flags, vmIP saddr, vmIP daddr, bool enableReload = false);
	static void loadActive(SqlDb *sqlDb = NULL);
	static void freeActive();
	static void prepareReload();
	static void applyReload();
	static void lock() {
		while(__sync_lock_test_and_set(&_sync, 1));
	}
	static void unlock() {
		__sync_lock_release(&_sync);
	}
	static void lock_reload() {
		while(__sync_lock_test_and_set(&_sync_reload, 1));
	}
	static void unlock_reload() {
		__sync_lock_release(&_sync_reload);
	}
private:
	int count;
	static IPfilter *filter_active;
	static IPfilter *filter_reload;
	static volatile bool reload_do;
	static volatile int _sync;
	static volatile int _sync_reload;
};

class TELNUMfilter : public filter_base {
private:
	struct db_row : filter_db_row_base {
		db_row() {
			memset(prefix, 0, sizeof(prefix));
		}
		char prefix[MAX_PREFIX];
	};
	struct t_payload {
		char prefix[MAX_PREFIX];
		int direction;
		unsigned int ip;
		int mask;
		unsigned long int flags;
	};
        struct t_node_tel {
                t_node_tel *nodes[256];
                t_payload *payload;
        };
        t_node_tel *first_node;
public: 
        TELNUMfilter();
        ~TELNUMfilter();
        void load(SqlDb *sqlDb = NULL);
	void loadFile();
	void add_payload(t_payload *payload);
	int _add_call_flags(volatile unsigned long int *flags, char *telnum_src, char *telnum_dst);
        static void dump2man(ostringstream &oss, t_node_tel *node = NULL);
	static int add_call_flags(volatile unsigned long int *flags, char *telnum_src, char *telnum_dst, bool enableReload = false);
	static void loadActive(SqlDb *sqlDb = NULL);
	static void freeActive();
	static void prepareReload();
	static void applyReload();
	static void lock() {
		while(__sync_lock_test_and_set(&_sync, 1));
	}
	static void unlock() {
		__sync_lock_release(&_sync);
	}
	static void lock_reload() {
		while(__sync_lock_test_and_set(&_sync_reload, 1));
	}
	static void unlock_reload() {
		__sync_lock_release(&_sync_reload);
	}
private:
	int count;
	static TELNUMfilter *filter_active;
	static TELNUMfilter *filter_reload;
	static volatile bool reload_do;
	static volatile int _sync;
	static volatile int _sync_reload;
};

class DOMAINfilter : public filter_base {
private:
	struct db_row : filter_db_row_base{
		db_row() {
		}
		std::string domain;
	};
        struct t_node {
		std::string domain;
		int direction;
		unsigned long int flags;
		t_node *next;
	};
	t_node *first_node;
public: 
	DOMAINfilter();
	~DOMAINfilter();
	void load(SqlDb *sqlDb = NULL);
	int _add_call_flags(volatile unsigned long int *flags, char *domain_src, char *domain_dst);
        static void dump2man(ostringstream &oss);
	static int add_call_flags(volatile unsigned long int *flags, char *domain_src, char *domain_dst, bool enableReload = false);
	static void loadActive(SqlDb *sqlDb = NULL);
	static void freeActive();
	static void prepareReload();
	static void applyReload();
	static void lock() {
		while(__sync_lock_test_and_set(&_sync, 1));
	}
	static void unlock() {
		__sync_lock_release(&_sync);
	}
	static void lock_reload() {
		while(__sync_lock_test_and_set(&_sync_reload, 1));
	}
	static void unlock_reload() {
		__sync_lock_release(&_sync_reload);
	}
private:
	int count;
	static DOMAINfilter *filter_active;
	static DOMAINfilter *filter_reload;
	static volatile bool reload_do;
	static volatile int _sync;
	static volatile int _sync_reload;
};

class SIP_HEADERfilter : public filter_base {
private:
	struct db_row : filter_db_row_base{
		db_row() {
		}
		std::string header;
		std::string content;
		bool prefix;
		bool regexp;
	};
        struct item_data {
		int direction;
		bool prefix;
		bool regexp;
		unsigned long int flags;
	};
	struct header_data {
		std::map<std::string, item_data> strict_prefix;
		std::map<std::string, item_data> regexp;
	};
	std::map<std::string, header_data> data;
public: 
	SIP_HEADERfilter();
	~SIP_HEADERfilter();
	void load(SqlDb *sqlDb = NULL);
	void loadFile();
	int _add_call_flags(struct ParsePacket::ppContentsX *parseContents, volatile unsigned long int *flags);
        static void dump2man(ostringstream &oss);
	void _addNodes(ParsePacket *parsePacket);
	static int add_call_flags(struct ParsePacket::ppContentsX *parseContents, volatile unsigned long int *flags, bool enableReload = false);
	static void addNodes(ParsePacket *parsePacket);
	static void loadActive(SqlDb *sqlDb = NULL);
	static void freeActive();
	static void prepareReload();
	static void applyReload();
	static unsigned long getLoadTime() {
		return(loadTime);
	}
	static void lock() {
		while(__sync_lock_test_and_set(&_sync, 1));
	}
	static void unlock() {
		__sync_lock_release(&_sync);
	}
	static void lock_reload() {
		while(__sync_lock_test_and_set(&_sync_reload, 1));
	}
	static void unlock_reload() {
		__sync_lock_release(&_sync_reload);
	}
private:
	int count;
	static SIP_HEADERfilter *filter_active;
	static SIP_HEADERfilter *filter_reload;
	static volatile bool reload_do;
	static volatile unsigned long loadTime;
	static volatile int _sync;
	static volatile int _sync_reload;
};

inline void set_global_flags(volatile unsigned long int &flags) {
	extern int opt_saveSIP;
	extern int opt_saveRTP;
	extern int opt_onlyRTPheader;
	extern int opt_saveRTCP;
	extern int opt_saveWAV;
	extern int opt_audio_format;
	extern int opt_saveGRAPH;
	extern int opt_skipdefault;
	extern int opt_hide_message_content;
	extern int opt_dbdtmf;
	extern int opt_pcapdtmf;
	extern bool opt_sip_register_save_all;
	extern int opt_sip_options;
	extern int opt_sip_subscribe;
	extern int opt_sip_notify;
	extern int opt_save_sip_options;
	extern int opt_save_sip_subscribe;
	extern int opt_save_sip_notify;

	if(opt_saveSIP) {
		flags |= FLAG_SAVESIP;
	}
	if(opt_saveRTP) {
		flags |= FLAG_SAVERTP;
	}
	if(opt_onlyRTPheader) {
		flags |= FLAG_SAVERTPHEADER;
	}
	if(opt_saveRTCP) {
		flags |= FLAG_SAVERTCP;
	}
	if(opt_saveWAV) {
		flags |= (opt_audio_format == FORMAT_OGG ? FLAG_SAVEAUDIO_OGG : FLAG_SAVEAUDIO_WAV);
	}
	if(opt_saveGRAPH) {
		flags |= FLAG_SAVEGRAPH;
	}
	if(opt_skipdefault) {
		flags |= FLAG_SKIPCDR;
	}
	if(opt_hide_message_content) {
		flags |= FLAG_HIDEMESSAGE;
	}
	if(opt_sip_register_save_all) {
		flags |= FLAG_SAVEREGISTER;
	}
	if (opt_dbdtmf) {
		flags |= FLAG_SAVEDTMFDB;
	}
	if (opt_pcapdtmf) {
		flags |= FLAG_SAVEDTMFPCAP;
	}
	if (opt_sip_options == 1) {
		flags |= FLAG_SAVEOPTIONSDB;
	}
	if (opt_sip_options && opt_save_sip_options) {
		flags |= FLAG_SAVEOPTIONSPCAP;
	}
	if (opt_sip_notify == 1) {
		flags |= FLAG_SAVENOTIFYDB;
	}
	if (opt_sip_notify && opt_save_sip_notify) {
		flags |= FLAG_SAVENOTIFYPCAP;
	}
	if (opt_sip_subscribe == 1) {
		flags |= FLAG_SAVESUBSCRIBEDB;
	}
	if (opt_sip_subscribe && opt_save_sip_subscribe) {
		flags |= FLAG_SAVESUBSCRIBEPCAP;
	}
}


#endif //FILTER_MYSQL_H

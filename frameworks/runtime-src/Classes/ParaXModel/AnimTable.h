#pragma once
#include <string>
#include <map>

namespace ParaEngine
{

	/** table for character animation. */
	class CAnimTable
	{
	private:
		std::map<int, int> m_AnimDefaultMaps;
		std::map<std::string, int> m_AnimNameMaps;
	public:
		CAnimTable(void);
		~CAnimTable(void);
	public:
		/** 
		*@param sName: name of the animation.
		*@return: the index of the animID. If no animation matches the name, 0 is returned
		*/
		static int GetAnimIDByName(const char* sName);
		/** 
		* if the nID does not exists, then a default ID will be used instead.
		* e.g. if there is no swimming, running is used, if there is no running walking is used.
		* if there is no walking, standing is used. If not found, standing animation is used.
		* for an entire, default hierarchy, please see the function body.
		*@param nID: nID
		*@return: default ID returned.
		*/
		static int GetDefaultAnimIDof(int nID);
		/** check if nID can default to any move animation id like walk. */
		static bool IsWalkAnimation(int nID);
	public:
		/** singleton instance.*/
		static CAnimTable* GetInstance();
	};

	/** this is just an ad hoc mapping from animation ID to its function*/
	enum AnimationID
	{
		ANIM_STAND=0,
		ANIM_DEATH=1,
		ANIM_SPELL=2,
		ANIM_STOP=3,
		ANIM_WALK=4,
		ANIM_RUN=5,
		ANIM_DEAD=6,
		ANIM_RISE=7,
		ANIM_STANDWOUND=8,
		ANIM_COMBATWOUND=9,
		ANIM_COMBATCRITICAL=10,
		ANIM_SHUFFLELEFT=11,
		ANIM_SHUFFLERIGHT=12,
		ANIM_WALKBACKWARDS=13,
		ANIM_STUN=14,
		ANIM_HANDSCLOSED=15,
		ANIM_ATTACKUNARMED=16,
		ANIM_ATTACK1H=17,
		ANIM_ATTACK2H=18,
		ANIM_ATTACK2HL=19,
		ANIM_PARRYUNARMED=20,
		ANIM_PARRY1H=21,
		ANIM_PARRY2H=22,
		ANIM_PARRY2HL=23,
		ANIM_SHIELDBLOCK=24,
		ANIM_READYUNARMED=25,
		ANIM_READY1H=26,
		ANIM_READY2H=27,
		ANIM_READY2HL=28,
		ANIM_READYBOW=29,
		ANIM_DODGE=30,
		ANIM_SPELLPRECAST=31,
		ANIM_SPELLCAST=32,
		ANIM_SPELLCASTAREA=33,
		ANIM_NPCWELCOME=34,
		ANIM_NPCGOODBYE=35,
		ANIM_BLOCK=36,
		ANIM_JUMPSTART=37,
		ANIM_JUMP=38,
		ANIM_JUMPEND=39,
		ANIM_FALL=40,
		ANIM_SWIMIDLE=41,
		ANIM_SWIM=42,
		ANIM_SWIMLEFT=43,
		ANIM_SWIMRIGHT=44,
		ANIM_SWIMBACKWARDS=45,
		ANIM_ATTACKBOW=46,
		ANIM_FIREBOW=47,
		ANIM_READYRIFLE=48,
		ANIM_ATTACKRIFLE=49,
		ANIM_LOOT=50,
		ANIM_READYSPELLDIRECTED=51,
		ANIM_READYSPELLOMNI=52,
		ANIM_SPELLCASTDIRECTED=53,
		ANIM_SPELLCASTOMNI=54,
		ANIM_BATTLEROAR=55,
		ANIM_READYABILITY=56,
		ANIM_SPECIAL1H=57,
		ANIM_SPECIAL2H=58,
		ANIM_SHIELDBASH=59,
		ANIM_EMOTETALK=60,
		ANIM_EMOTEEAT=61,
		ANIM_EMOTEWORK=62,
		ANIM_EMOTEUSESTANDING=63,
		ANIM_EMOTETALKEXCLAMATION=64,
		ANIM_EMOTETALKQUESTION=65,
		ANIM_EMOTEBOW=66,
		ANIM_EMOTEWAVE=67,
		ANIM_EMOTECHEER=68,
		ANIM_EMOTEDANCE=69,
		ANIM_EMOTELAUGH=70,
		ANIM_EMOTESLEEP=71,
		ANIM_EMOTESITGROUND=72,
		ANIM_EMOTERUDE=73,
		ANIM_EMOTEROAR=74,
		ANIM_EMOTEKNEEL=75,
		ANIM_EMOTEKISS=76,
		ANIM_EMOTECRY=77,
		ANIM_EMOTECHICKEN=78,
		ANIM_EMOTEBEG=79,
		ANIM_EMOTEAPPLAUD=80,
		ANIM_EMOTESHOUT=81,
		ANIM_EMOTEFLEX=82,
		ANIM_EMOTESHY=83,
		ANIM_EMOTEPOINT=84,
		ANIM_ATTACK1HPIERCE=85,
		ANIM_ATTACK2HLOOSEPIERCE=86,
		ANIM_ATTACKOFF=87,
		ANIM_ATTACKOFFPIERCE=88,
		ANIM_SHEATH=89,
		ANIM_HIPSHEATH=90,
		ANIM_MOUNT=91,
		ANIM_RUNRIGHT=92,
		ANIM_RUNLEFT=93,
		ANIM_MOUNTSPECIAL=94,
		ANIM_KICK=95,
		ANIM_SITGROUNDDOWN=96,
		ANIM_SITGROUND=97,
		ANIM_SITGROUNDUP=98,
		ANIM_SLEEPDOWN=99,
		ANIM_SLEEP=100,
		ANIM_SLEEPUP=101,
		ANIM_SITCHAIRLOW=102,
		ANIM_SITCHAIRMED=103,
		ANIM_SITCHAIRHIGH=104,
		ANIM_LOADBOW=105,
		ANIM_LOADRIFLE=106,
		ANIM_ATTACKTHROWN=107,
		ANIM_READYTHROWN=108,
		ANIM_HOLDBOW=109,
		ANIM_HOLDRIFLE=110,
		ANIM_HOLDTHROWN=111,
		ANIM_LOADTHROWN=112,
		ANIM_EMOTESALUTE=113,
		ANIM_KNEELSTART=114,
		ANIM_KNEELLOOP=115,
		ANIM_KNEELEND=116,
		ANIM_ATTACKUNARMEDOFF=117,
		ANIM_SPECIALUNARMED=118,
		ANIM_STEALTHWALK=119,
		ANIM_STEALTHSTAND=120,
		ANIM_KNOCKDOWN=121,
		ANIM_EATINGLOOP=122,
		ANIM_USESTANDINGLOOP=123,
		ANIM_CHANNELCASTDIRECTED=124,
		ANIM_CHANNELCASTOMNI=125,
		ANIM_WHIRLWIND=126,
		ANIM_BIRTH=127,
		ANIM_USESTANDINGSTART=128,
		ANIM_USESTANDINGEND=129,
		ANIM_CREATURESPECIAL=130,
		ANIM_DROWN=131,
		ANIM_DROWNED=132,
		ANIM_FISHINGCAST=133,
		ANIM_FISHINGLOOP=134,
		ANIM_FLY=135,
		ANIM_EMOTEWORKNOSHEATHE=136,
		ANIM_EMOTESTUNNOSHEATHE=137,
		ANIM_EMOTEUSESTANDINGNOSHEATHE=138,
		ANIM_SPELLSLEEPDOWN=139,
		ANIM_SPELLKNEELSTART=140,
		ANIM_SPELLKNEELLOOP=141,
		ANIM_SPELLKNEELEND=142,
		ANIM_SPRINT=143,
		ANIM_INFLIGHT=144,
		ANIM_SPAWN=145,
		ANIM_CLOSE=146,
		ANIM_CLOSED=147,
		ANIM_OPEN=148,
		ANIM_OPENED=149,
		ANIM_DESTROY=150,
		ANIM_DESTROYED=151,
		ANIM_REBUILD=152,
		ANIM_CUSTOM0=153,
		ANIM_CUSTOM1=154,
		ANIM_CUSTOM2=155,
		ANIM_CUSTOM3=156,
		ANIM_DESPAWN=157,
		ANIM_HOLD=158,
		ANIM_DECAY=159,
		ANIM_BOWPULL=160,
		ANIM_BOWRELEASE=161,
		ANIM_SHIPSTART=162,
		ANIM_SHIPMOVING=163,
		ANIM_SHIPSTOP=164,
		ANIM_GROUPARROW=165,
		ANIM_ARROW=166,
		ANIM_CORPSEARROW=167,
		ANIM_GUIDEARROW=168,
		ANIM_SWAY=169,
		ANIM_DRUIDCATPOUNCE=170,
		ANIM_DRUIDCATRIP=171,
		ANIM_DRUIDCATRAKE=172,
		ANIM_DRUIDCATRAVAGE=173,
		ANIM_DRUIDCATCLAW=174,
		ANIM_DRUIDCATCOWER=175,
		ANIM_DRUIDBEARSWIPE=176,
		ANIM_DRUIDBEARBITE=177,
		ANIM_DRUIDBEARMAUL=178,
		ANIM_DRUIDBEARBASH=179,
		ANIM_DRAGONTAIL=180,
		ANIM_DRAGONSTOMP=181,
		ANIM_DRAGONSPIT=182,
		ANIM_DRAGONSPITHOVER=183,
		ANIM_DRAGONSPITFLY=184,
		ANIM_EMOTEYES=185,
		ANIM_EMOTENO=186,
		ANIM_JUMPLANDRUN=187,
		ANIM_LOOTHOLD=188,
		ANIM_LOOTUP=189,
		ANIM_STANDHIGH=190,
		ANIM_IMPACT=191,
		ANIM_LIFTOFF=192,
		ANIM_HOVER=193,
		ANIM_SUCCUBUSENTICE=194,
		ANIM_EMOTETRAIN=195,
		ANIM_EMOTEDEAD=196,
		ANIM_EMOTEDANCEONCE=197,
		ANIM_DEFLECT=198,
		ANIM_EMOTEEATNOSHEATHE=199,
		ANIM_LAND=200,
		ANIM_SUBMERGE=201,
		ANIM_SUBMERGED=202,
		ANIM_CANNIBALIZE=203,
		ANIM_ARROWBIRTH=204,
		ANIM_GROUPARROWBIRTH=205,
		ANIM_CORPSEARROWBIRTH=206,
		ANIM_GUIDEARROWBIRTH=207,
		/*
		ANIM_STAND = 0,
		ANIM_DEATH = 1,
		ANIM_SPELL = 2,
		ANIM_STOP = 3,
		ANIM_WALK = 4,
		ANIM_RUN = 5,
		ANIM_RUN_LEFT = 11,	
		ANIM_RUN_RIGHT = 12,
		ANIM_WALK_BACKWORD = 13,
		ANIM_ATTACKUARMED = 16,
		ANIM_ATTACK1H = 17,
		ANIM_ATTACK2H = 18,
		ANIM_ATTACK2HL = 19,
		ANIM_PARRYUARMED = 20,
		ANIM_PARRY1H = 21,
		ANIM_JUMP_START = 37,
		ANIM_JUMP = 38,
		ANIM_JUMP_END = 39,
		ANIM_FALL = 40,
		ANIM_SWIM_IDLE = 41,
		ANIM_SWIM = 42,
		ANIM_SWIM_RIGHT = 43,
		ANIM_SWIM_LEFT = 44,
		ANIM_SWIM_BACK = 45,
		ANIM_EMOTE_TALK = 60,
		ANIM_EMOTE_DANCE = 69,
		ANIM_MOUNT = 91,
		ANIM_SIT_GROUNDDOWN = 96,
		ANIM_SIT_GROUND = 97,
		ANIM_SIT_GROUNDUP = 98,
		ANIM_SIT_SLEEPDOWN = 99,
		ANIM_SIT_SLEEP = 100,
		ANIM_SIT_SLEEPUP = 101,
		ANIM_EMOTE_YES = 185,
		ANIM_EMOTE_NO = 186,
		ANIM_LAND = 200
		*/
		ANIM_LASTONE
	};
}

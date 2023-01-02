#include <iostream>
#include "dllmain.h"
#include "Game.h"
#include "Settings.h"
#include "ConsoleWnd.h"
#include "NTSC.h"

//int* Dmg_tbl_sml;
int* Dmg_tbl_em2b;
int* Dmg_tbl_em2c;
int* Dmg_tbl_em2d;
int* Dmg_tbl_em31;
int* Dmg_tbl_em32;
int* Dmg_tbl_em36;
int* Dmg_tbl_em39;
//int* Dmg_tbl_em3c;
//int* Dmg_tbl_em3f;
//int* Dmg_tbl_em4e;
int* Dmg_tbl_em10;

float* WeaponLevelTbl;
float* PlShotFrameTbl;
float* PlReloadSpeedTbl;
float* PlReloadEndTbl;

PRICE_INFO* g_item_price_tbl;
LEVEL_PRICE* level_price;

static uint8_t RandomItemCk_em_id = 0;
static uint32_t RandomItemCk_ctrl_flag = 0;

void GetMerchantPointers()
{
	auto pattern = hook::pattern("68 ? ? ? ? 68 ? ? ? ? 68 ? ? ? ? 68 ? ? ? ? 68");
	level_price = *pattern.count(1).get(0).get<LEVEL_PRICE*>(1);
	g_item_price_tbl = *pattern.count(1).get(0).get<PRICE_INFO*>(6);
}

void GetDamageTablePointers()
{
	auto pattern = hook::pattern("83 C0 F0 83 F8 3F 77 ? FF ? ? ? ? ? ? 8B");
//  Dmg_tbl_em4e = *pattern.count(1).get(0).get<int*>(18);      // ?
//  Dmg_tbl_sml = *pattern.count(1).get(0).get<int*>(18+9*1);   // OHKO
	Dmg_tbl_em2b = *pattern.count(1).get(0).get<int*>(18+9*2);  // El Gigante
	Dmg_tbl_em2c = *pattern.count(1).get(0).get<int*>(18+9*3);  // Verdrugo
	Dmg_tbl_em2d = *pattern.count(1).get(0).get<int*>(18+9*4);  // Novistadors
	Dmg_tbl_em31 = *pattern.count(1).get(0).get<int*>(18+9*5);  // Spider Saddler
	Dmg_tbl_em32 = *pattern.count(1).get(0).get<int*>(18+9*6);  // U3, others?
	Dmg_tbl_em36 = *pattern.count(1).get(0).get<int*>(18+9*7);  // Regenerators
	Dmg_tbl_em39 = *pattern.count(1).get(0).get<int*>(18+9*8);  // Krauser
//  Dmg_tbl_em3c = *pattern.count(1).get(0).get<int*>(18+9*9);  // Knight Armor, identical to GC
//  Dmg_tbl_em3f = *pattern.count(1).get(0).get<int*>(18+9*10); // Saddler (Separate Ways)
	Dmg_tbl_em10 = *pattern.count(1).get(0).get<int*>(18+9*11); // Ganados
}

void GetWeaponStatsPointers()
{
	auto pattern = hook::pattern("D9 ? ? ? ? ? ? D9 ? ? 75 ? 8B CE 83");
	WeaponLevelTbl = *pattern.count(1).get(0).get<float*>(3);
	pattern = hook::pattern("8D 04 80 03 C1 51 D9");
	PlShotFrameTbl = *pattern.count(1).get(0).get<float*>(9);
	pattern = hook::pattern("8D 14 41 03 D0 5E D9");
	PlReloadSpeedTbl = *pattern.count(1).get(0).get<float*>(9);
	pattern = hook::pattern("8D 04 40 03 C1 D9");
	PlReloadEndTbl = *pattern.count(1).get(0).get<float*>(8);
}

bool(__cdecl* MotionCheckCrossFrame)(MOTION_INFO* pInfo, float frame);
bool __cdecl wep17_r3_fire10_MotionCheckCrossFrame_hook1(MOTION_INFO* pInfo, float frame)
{
	return MotionCheckCrossFrame(pInfo, 3.0f);
}
bool __cdecl wep17_r3_fire10_MotionCheckCrossFrame_hook2(MOTION_INFO* pInfo, float frame)
{
	return MotionCheckCrossFrame(pInfo, 11.0f);
}

BOOL(__cdecl* RandomItemCk)(uint8_t em_id, uint32_t* ret_id, uint32_t* ret_num, uint32_t ctrl_flag);
BOOL __cdecl RandomItemCk_hook(uint8_t em_id, uint32_t* ret_id, uint32_t* ret_num, uint32_t ctrl_flag)
{
	RandomItemCk_em_id = em_id;
	RandomItemCk_ctrl_flag = ctrl_flag;
	return RandomItemCk(em_id, ret_id, ret_num, ctrl_flag);
}

uint32_t __cdecl GetBulletPoint_ntsc()
{
	uint16_t curChapter = HIBYTE(GlobalPtr()->curRoomId_4FAC);

	return (curChapter <= 1
		? ItemMgr->bulletNumTotal((ITEM_ID)EItemId::Bullet_9mm_H) // GC handgun ammo has no coefficient during chapter 1
		: ItemMgr->bulletNumTotal((ITEM_ID)EItemId::Bullet_9mm_H) / 2 + 1)

		+ ItemMgr->bulletNumTotal((ITEM_ID)EItemId::Bullet_12gg) * 2 + 1 // 2x coefficient on shotgun ammo in GC, vs 4x in UHD
		+ ItemMgr->bulletNumTotal((ITEM_ID)EItemId::Bullet_9mm_M) / 5 + 1 // (1/5)x coefficient on TMP Ammo in GC, vs (1/3)x in UHD
		+ (ItemMgr->bulletNumTotal((ITEM_ID)EItemId::Bullet_Arrow == 0) ? 0 : 5);
}

BOOL(__cdecl* GetDropBullet_orig)(uint32_t* ret_id, uint32_t* ret_num);
BOOL __cdecl GetDropBullet_ntsc(uint32_t* ret_id, uint32_t* ret_num)
{
	auto dropItem = [&](EItemId itemId, int stackSize = 0)
	{
		*ret_id = (uint32_t)itemId;
		*ret_num = stackSize; // ret_num = 0 creates an item with its defNum_3 stack size
		return TRUE;
	};

	if (GetBulletPoint_ntsc() >= 150)
		return FALSE;

	uint32_t result = bio4::Rnd() % 100;

	bool isAssignmentAda = FlagIsSet(GlobalPtr()->Flags_SYSTEM_0_54, uint32_t(Flags_SYSTEM::SYS_OMAKE_ADA_GAME));

	if (isAssignmentAda)
	{
		if (result < 40)
			return dropItem(EItemId::Bullet_9mm_H, bio4::Rnd() % 10 < 7 ? 10 : 15);
		else if (result < 90)
			return dropItem(EItemId::Bullet_9mm_M, bio4::Rnd() % 10 < 6 ? 25 : 50);
		else if (result < 95)
			return dropItem(EItemId::Bullet_223in, bio4::Rnd() % 10 < 8 ? 3 : 5);
		else
			return dropItem(EItemId::Grenade);
	}

	bool isMercenaries = FlagIsSet(GlobalPtr()->Flags_SYSTEM_0_54, uint32_t(Flags_SYSTEM::SYS_OMAKE_ETC_GAME));

	if (isMercenaries)
	{
		PlayerCharacter curPlType = GlobalPtr()->pl_type_4FC8;

		if (curPlType == PlayerCharacter::Leon)
		{
			if (result < 55)
				return dropItem(EItemId::Bullet_9mm_H);
			else if (result < 90)
				return dropItem(EItemId::Bullet_12gg, (bio4::Rnd() & 15) != 5 ? 2 * (bio4::Rnd() % 10 < 5) + 3 : 10);
			else
				return dropItem(EItemId::Grenade);
		}
		else if (curPlType == PlayerCharacter::Ada)
		{
			if (result < 30)
				return dropItem(EItemId::Bullet_9mm_H);
			else if (result < 65)
				return dropItem(EItemId::Bullet_9mm_M, bio4::Rnd() % 10 < 6 ? 25 : 50);
			else if (result < 85)
				return dropItem(EItemId::Flame_Grenade);
			else
				return dropItem(EItemId::Bullet_223in, bio4::Rnd() % 10 < 8 ? 3 : 5);
		}
		else if (curPlType == PlayerCharacter::HUNK)
		{
			if (result < 75)
				return dropItem(EItemId::Bullet_9mm_M, bio4::Rnd() % 10 < 6 ? 25 : 50);
			else
				return dropItem(EItemId::Grenade);
		}
		else if (curPlType == PlayerCharacter::Krauser)
		{
			if (result < 75)
				return dropItem(EItemId::Bullet_Arrow, bio4::Rnd() % 10 < 6 ? 5 : 10);
			else
				return dropItem(EItemId::Light_Grenade);
		}
		else//if (curPlType == PlayerCharacter::Wesker)
		{
			if (result < 25)
				return dropItem(EItemId::Bullet_9mm_H);
			//if (result < 70)
			//	dropItem(EItemId::Bullet_45in_H, bio4::Rnd() % 10 < 8 ? 2 : 5);
			if (result >= 80)
			{
				if (result >= 95)
					return dropItem(EItemId::Flame_Grenade);
				else if (result >= 90)
					return dropItem(EItemId::Light_Grenade);
				else
					return dropItem(EItemId::Grenade);
			}
			// bug in the original game logic? magnum ammo doesn't return early, so rifle ammo always overwrites it
			else
				return dropItem(EItemId::Bullet_223in, bio4::Rnd() % 10 < 8 ? 3 : 5);
		}
	}

	result = bio4::Rnd() % 100;

	int curHandgunAmmo = ItemMgr->num((ITEM_ID)EItemId::Bullet_9mm_H);

	if (result <= 40 || curHandgunAmmo >= 60)
	{
		bool hasShotgun = ItemMgr->num((ITEM_ID)EItemId::Shotgun) || ItemMgr->num((ITEM_ID)EItemId::Riot_Gun) || ItemMgr->num((ITEM_ID)EItemId::Striker);

		if (result < 20 && hasShotgun)
			return dropItem(EItemId::Bullet_12gg, (bio4::Rnd() & 15) != 5 ? 2 * (bio4::Rnd() % 10 < 5) + 3 : 10);

		bool hasRifle = ItemMgr->num((ITEM_ID)EItemId::S_Field) || ItemMgr->num((ITEM_ID)EItemId::HK_Sniper);

		if (result - 20 < 20 && hasRifle)
			return dropItem(EItemId::Bullet_223in, bio4::Rnd() % 10 < 8 ? 3 : 5);

		bool hasTMP = ItemMgr->num((ITEM_ID)EItemId::Styer) || ItemMgr->num((ITEM_ID)EItemId::Krauser_Machine_Gun);

		if (result - 40 < 20 && hasTMP)
			return dropItem(EItemId::Bullet_9mm_M, bio4::Rnd() % 10 < 6 ? 25 : 50);

		bool hasHandcannon = ItemMgr->num((ITEM_ID)EItemId::SW500);
		bool hasMagnum = ItemMgr->num((ITEM_ID)EItemId::Gov) || ItemMgr->num((ITEM_ID)EItemId::Civilian);

		if (result - 60 < 20)
		{
			if (hasHandcannon && bio4::Rnd() % 10 > 1)
				return dropItem(EItemId::Bullet_5in, bio4::Rnd() % 10 < 8 ? 2 : 10);
			if (hasMagnum)
				return dropItem(EItemId::Bullet_45in_H, bio4::Rnd() % 10 < 8 ? 2 : 5);
		}

		bool hasMineThrower = ItemMgr->num((ITEM_ID)EItemId::Mine);

		if (result - 80 < 20 && hasMineThrower)
			return dropItem(EItemId::Bullet_Mine_A, bio4::Rnd() % 10 < 8 ? 2 : 1);

		bool isSeparateWays = FlagIsSet(GlobalPtr()->Flags_SYSTEM_0_54, uint32_t(Flags_SYSTEM::SYS_OMAKE_ADA_GAME));
		bool hasBowgun = ItemMgr->num((ITEM_ID)EItemId::Bow_Gun);

		if (result - 90 < 10)
		{
			// there was no Separate Ways in this version of the game, so this is just some perfunctory support
			if (isSeparateWays && hasBowgun && bio4::Rnd() % 100 < 75)
				return dropItem(EItemId::Bullet_Bow_Gun);

			result = bio4::Rnd() % 3;
			switch (result)
			{
			case 0: return dropItem(EItemId::Grenade);
			case 1: return dropItem(EItemId::Flame_Grenade);
			case 2: return dropItem(EItemId::Light_Grenade);
			}
		}

		// 2nd round of rolls

		if (bio4::Rnd() % 10 > 2 && hasShotgun)
			return dropItem(EItemId::Bullet_12gg, (bio4::Rnd() & 15) != 5 ? 2 * (bio4::Rnd() % 10 < 5) + 3 : 10);

		if (bio4::Rnd() % 10 > 4 && hasTMP)
			return dropItem(EItemId::Bullet_9mm_M, bio4::Rnd() % 10 < 6 ? 25 : 50);

		if (bio4::Rnd() % 10 > 4 && hasRifle)
			return dropItem(EItemId::Bullet_223in, (bio4::Rnd() % 10 > 8) + 2);

		if (bio4::Rnd() % 10 > 4 && hasMineThrower)
			return dropItem(EItemId::Bullet_Mine_A, ~((bio4::Rnd() % 10 < 8) - 1) & 4);

		if (bio4::Rnd() % 10 > 4 && hasHandcannon)
			return dropItem(EItemId::Bullet_5in, bio4::Rnd() % 10 < 8 ? 2 : 10);

		if (bio4::Rnd() % 10 > 4 && hasMagnum)
			return dropItem(EItemId::Bullet_45in_H, bio4::Rnd() % 10 < 8 ? 2 : 5);

		if (bio4::Rnd() % 10 > 7)
		{
			if (isSeparateWays && hasBowgun && bio4::Rnd() % 100 < 75)
				return dropItem(EItemId::Bullet_Bow_Gun);

			result = bio4::Rnd() % 3;
			switch (result)
			{
			case 0: return dropItem(EItemId::Grenade);
			case 1: return dropItem(EItemId::Flame_Grenade);
			case 2: return dropItem(EItemId::Light_Grenade);
			}
		}
	}

	uint16_t curChapter = HIBYTE(GlobalPtr()->curRoomId_4FAC);
	uint16_t curRoom = LOBYTE(GlobalPtr()->curRoomId_4FAC);

	return dropItem(EItemId::Bullet_9mm_H, curChapter == 1 || (curChapter == 5 && curRoom < 0xC) ? 10 : 20);
}


void re4t::init::NTSC()
{
	// NTSC mode
	// Enables difficulty modifiers previously exclusive to all North American console versions of RE4, including the recent gen 8 ports.
	// These were locked behind checks for pSys->language_8 == 1 (NTSC English). Since RE4 UHD uses PAL English (language_8 == 2), PC players never saw these.
	if (re4t::cfg->bEnableNTSCMode || re4t::cfg->bNAGameCubeBalance)
	{
		// Normal mode and Separate Ways: increased starting difficulty (3500->5500)
		auto pattern = hook::pattern("8A 50 ? FE CA 0F B6 C2");
		Patch(pattern.count(1).get(0).get<uint32_t>(0), { 0xB2, 0x01, 0x90 }); // GamePointInit, { mov dl, 1 }

		// Assignment Ada: increased difficulty (4500->6500)
		pattern = hook::pattern("66 39 B1 ? ? 00 00 75 10");
		injector::MakeNOP(pattern.count(1).get(0).get<uint32_t>(7), 2); // GameAddPoint

		// Shooting range: increased bottle cap score requirements (1000->3000)
		pattern = hook::pattern("8B F9 8A ? ? 8B ? ? FE C9");
		Patch(pattern.count(1).get(0).get<uint32_t>(2), { 0xB1, 0x01, 0x90 }); // cCap::check, { mov cl, 1 }

		// Shooting range: use NTSC strings for the game rules note
		// (only supports English for now, as only eng/ss_file_01.MDT contains the additional strings necessary for this)
		pattern = hook::pattern("? 00 01 00 46 00 01 00");
		struct FILE_MSG_TBL_mb {
			uint8_t top_0;
			uint8_t color_1;
			uint8_t attr_2;
			uint8_t layout_3;
		};
		static FILE_MSG_TBL_mb* file_msg_tbl_35 = pattern.count(1).get(0).get<FILE_MSG_TBL_mb>(0);
		// update the note's message index whenever we load into r22c
		pattern = hook::pattern("89 41 78 83 C1 7C E8");
		struct R22cInit_UpdateMsgIdx
		{
			void operator()(injector::reg_pack& regs)
			{
				file_msg_tbl_35[0].top_0 = SystemSavePtr()->language_8 == 2 ? 0x9D : 0x3F;

				// code we overwrote
				*(uint32_t*)(regs.ecx + 0x78) = regs.eax;
				regs.ecx += 0x7C;
			}
		}; injector::MakeInline<R22cInit_UpdateMsgIdx>(pattern.count(1).get(0).get<uint32_t>(0), pattern.count(1).get(0).get<uint32_t>(6));

		// Shooting range: only check for bottle cap reward once per results screen
		pattern = hook::pattern("8B 15 ? ? ? ? 80 7A ? 01 74");
		Patch(pattern.count(2).get(1).get<uint32_t>(10), { 0xEB }); // shootResult, jz -> jmp

		// Mercenaries: unlock village stage difficulty, requires 60fps fix
		Patch(pattern.count(2).get(0).get<uint32_t>(10), { 0xEB }); // GameAddPoint, jz -> jmp

		// remove Easy mode from the difficulty menu
		pattern = hook::pattern("A1 ? ? ? ? 80 78 ? 01 75");
		injector::MakeNOP(pattern.count(1).get(0).get<uint32_t>(9), 2); // titleLevelInit

		// swap NEW GAME texture for START on a fresh system save
		pattern = hook::pattern("89 51 68 8B 57 68 8B");
		struct titleMenuInit_StartTex
		{
			void operator()(injector::reg_pack& regs)
			{
				bool hasFinishedGame = FlagIsSet(SystemSavePtr()->flags_EXTRA_4, uint32_t(Flags_EXTRA::EXT_COSTUME));
				if (!hasFinishedGame)
				{
					float texW;

					// texW = aspect ratio of image file * size0_H_E0 (14)
					switch (SystemSavePtr()->language_8)
					{
					case 4: // French
						texW = 131.0f;
						break;
					case 5: // Spanish
						texW = 70.0f;
						break;
					case 6: // Traditional Chinese / Italian
						texW = GameVersion() == "1.1.0" ? 66.0f : 68.0f;
						break;
					case 8: // Italian
						texW = 68.0f;
						break;
					default: // English, German, Japanese, Simplified Chinese
						texW = 66.0f;
						break;
					}

					IDSystemPtr()->unitPtr(0x1u, IDC_TITLE_MENU)->texId_78 = 164;
					IDSystemPtr()->unitPtr(0x1u, IDC_TITLE_MENU)->size0_W_DC = texW;
					IDSystemPtr()->unitPtr(0x2u, IDC_TITLE_MENU)->texId_78 = 164;
					IDSystemPtr()->unitPtr(0x2u, IDC_TITLE_MENU)->size0_W_DC = texW;
				}

				// Code we overwrote
				*(uint32_t*)(regs.ecx + 0x68) = regs.edx;
				regs.edx = *(uint32_t*)(regs.edi + 0x68);
			}
		}; injector::MakeInline<titleMenuInit_StartTex>(pattern.count(1).get(0).get<uint32_t>(0), pattern.count(1).get(0).get<uint32_t>(6));

		// Patches for Japanese language support (language_8 == 0)

		// repurpose the hide Professional mode block to hide Amateur mode instead
		pattern = hook::pattern("C7 46 30 01 00 00 00");
		injector::MakeNOP(pattern.count(1).get(0).get<uint32_t>(7), 2); // titleLevelInit
		injector::MakeNOP(pattern.count(1).get(0).get<uint32_t>(16), 2);
		Patch(pattern.count(1).get(0).get<uint32_t>(21), { 0x09 });
		Patch(pattern.count(1).get(0).get<uint32_t>(38), { 0x0A });
		// erase the rest of the block
		pattern = hook::pattern("C7 46 ? 03 00 00 00 85 DB");
		injector::MakeNOP(pattern.get_first(0), 37);

		// disable JP difficulty select confirmation prompts
		pattern = hook::pattern("A1 ? ? ? ? 38 58 08 75");
		Patch(pattern.count(1).get(0).get<uint32_t>(8), { 0xEB }); // titleMain, jnz -> jmp

		// remove JP only 20% damage armor from Mercenaries mode
		pattern = hook::pattern("F7 46 54 00 00 00 40");
		Patch(pattern.count(1).get(0).get<uint32_t>(7), { 0xEB }); // LifeDownSet2, jz -> jmp

		spd::log()->info("NTSC mode enabled");
	}

	// Disable Ada's knife in Assignment Ada and Mercenaries
	// Ada wasn't given a knife in these modes until the Wii port
	{
		auto pattern = hook::pattern("88 ? 7D 84 00 00 E8");
		struct titleSub_DisableAdaKnife 
		{
			// Wii devs left in a hidden option to disable Ada's knife in titleSub by pressing game start with LT held down
			// Let's just overwrites that to check for our cfg option instead
			void operator()(injector::reg_pack& regs)
			{
				GlobalPtr()->joyLKamaeRelated_847D = re4t::cfg->bDisableAdaKnife ? 1 : 0;
			}
		};
		// hook Assignment Ada game start routine
		injector::MakeInline<titleSub_DisableAdaKnife>(pattern.count(1).get(0).get<uint32_t>(0), pattern.count(1).get(0).get<uint32_t>(6));
		// hook Mercenaries game start routine
		pattern = hook::pattern("6A 00 68 80 00 00 00 6A 01 E8 ? ? ? ? 83 C4 0C 84 C0 74");
		injector::MakeInline<titleSub_DisableAdaKnife>(pattern.count(1).get(0).get<uint32_t>(0), pattern.count(1).get(0).get<uint32_t>(34));
	}

	// Revert the 30% damage buff given to the knife since the Wii port
	// Presumably this was meant to make manually aiming the knife feel stronger than using quick knife
	{
		auto pattern = hook::pattern("A1 ? ? ? ? 8B 80 D8 07 00 00 85 C0 74");
		struct GetWepDmVal_DisableWiiKnifeBuff
		{
			void operator()(injector::reg_pack& regs)
			{
				bool disabled = re4t::cfg->bRevertWiiKnifeBuff || re4t::cfg->bNAGameCubeBalance;

				if (!disabled && PlayerPtr()->Wep_7D8 && PlayerPtr()->Wep_7D8->field_42 <= 0)
					regs.ef |= (1 << regs.zero_flag);
				else
					regs.ef &= ~(1 << regs.zero_flag);
			}
		};
		injector::MakeInline<GetWepDmVal_DisableWiiKnifeBuff>(pattern.count(1).get(0).get<uint32_t>(0), pattern.count(1).get(0).get<uint32_t>(19));
	}

	GetMerchantPointers();
	GetDamageTablePointers();
	GetWeaponStatsPointers();

	// GameCube-NTSC Mode
	// The original North American GameCube release of RE4 was an earlier build of the game with its own unique gameplay quirks
	// Notably, ammo drops more often, pesetas drop less often, the knife does 60% less damage, the TMP does 30% less damage to Ganados, many weapon stats and item prices are different
	if (re4t::cfg->bNAGameCubeBalance)
	{
		// Restore GameCube-NTSC merchant prices
		injector::WriteMemoryRaw(g_item_price_tbl, (void*)g_item_price_tbl_ntsc, sizeof(g_item_price_tbl_ntsc), true);
		injector::WriteMemoryRaw(level_price, (void*)level_price_ntsc, sizeof(level_price_ntsc), true);

		// Restore GameCube-NTSC damage tables
		injector::WriteMemoryRaw(Dmg_tbl_em2b, (void*)Dmg_tbl_em2b_ntsc, sizeof(Dmg_tbl_em2b_ntsc), true);
		injector::WriteMemoryRaw(Dmg_tbl_em2c, (void*)Dmg_tbl_em2c_ntsc, sizeof(Dmg_tbl_em2c_ntsc), true);
		injector::WriteMemoryRaw(Dmg_tbl_em2d, (void*)Dmg_tbl_em2d_ntsc, sizeof(Dmg_tbl_em2d_ntsc), true);
		injector::WriteMemoryRaw(Dmg_tbl_em31, (void*)Dmg_tbl_em31_ntsc, sizeof(Dmg_tbl_em31_ntsc), true);
		injector::WriteMemoryRaw(Dmg_tbl_em32, (void*)Dmg_tbl_em3c_ntsc, sizeof(Dmg_tbl_em3c_ntsc), true);
		injector::WriteMemoryRaw(Dmg_tbl_em36, (void*)Dmg_tbl_em36_ntsc, sizeof(Dmg_tbl_em36_ntsc), true);
		injector::WriteMemoryRaw(Dmg_tbl_em39, (void*)Dmg_tbl_em39_ntsc, sizeof(Dmg_tbl_em39_ntsc), true);
		injector::WriteMemoryRaw(Dmg_tbl_em10, (void*)Dmg_tbl_em10_ntsc, sizeof(Dmg_tbl_em10_ntsc), true);

		// Restore GameCube-NTSC weapon stats
		injector::WriteMemoryRaw(WeaponLevelTbl, (void*)WeaponLevelTbl_ntsc, sizeof(WeaponLevelTbl_ntsc), true);
		injector::WriteMemoryRaw(PlShotFrameTbl, (void*)PlShotFrameTbl_ntsc, sizeof(PlShotFrameTbl_ntsc), true);
		injector::WriteMemoryRaw(PlReloadSpeedTbl, (void*)PlReloadSpeedTbl_ntsc, sizeof(PlReloadEndTbl_ntsc), true);
		injector::WriteMemoryRaw(PlReloadEndTbl, (void*)PlReloadEndTbl_ntsc, sizeof(PlReloadEndTbl_ntsc), true);

		spd::log()->info("Loaded GameCube-NTSC game tables");

		// The Merchant's First Aid Spray stock isn't dynamic and builds up with each visit
		auto pattern = hook::pattern("8B ? ? ? ? ? 80 B9 7C 84 00 00 01 76");
		injector::MakeNOP(pattern.count(1).get(0).get<uint32_t>(0), 43); // Merchant::stockNum 

		// Matilda has a slower rate of burst fire
		pattern = hook::pattern("E8 ? ? ? ? 83 C4 08 84 C0 74 ? 83 BE");
		ReadCall(pattern.count(1).get(0).get<uint8_t>(0), MotionCheckCrossFrame);
		InjectHook(pattern.count(1).get(0).get<uint32_t>(0), wep17_r3_fire10_MotionCheckCrossFrame_hook1, PATCH_CALL);
		pattern = hook::pattern("E8 ? ? ? ? 83 C4 08 84 C0 74 ? 8B 8E D8 07 00 00 C7");
		InjectHook(pattern.count(1).get(0).get<uint32_t>(0), wep17_r3_fire10_MotionCheckCrossFrame_hook2, PATCH_CALL);

		// Mine Thrower mines take 5 seconds to explode (vs 3 seconds in UHD)
		pattern = hook::pattern("D9 9E 24 04 00 00 D9 E8 D9");
		struct MineThrowerTimer
		{
			void operator()(injector::reg_pack& regs)
			{
				float* Bomb_wait_424 = (float*)(regs.esi + 0x424);
				*Bomb_wait_424 = 150.0f;
			}
		};
		// emMine_R1_Set
		injector::MakeInline<MineThrowerTimer>(pattern.count(2).get(0).get<uint32_t>(0), pattern.count(2).get(0).get<uint32_t>(6));
		// emMine_R1_Parent
		injector::MakeInline<MineThrowerTimer>(pattern.count(2).get(1).get<uint32_t>(0), pattern.count(2).get(1).get<uint32_t>(6));

		// The Handgun exclusive upgrade has a 50% crit chance (vs 33% in UHD)
		pattern = hook::pattern("B9 09 00 00 00 F7 F9 83");
		injector::WriteMemory(pattern.count(1).get(0).get<uint8_t>(1), uint8_t(0xC), true); // em10SetDmVal

		// U3 takes full damage from magnum weapons (vs 50% in UHD)
		pattern = hook::pattern("8A 8E 2E 03 00 00 80 E9 05 83");
		injector::MakeNOP(pattern.count(1).get(0).get<uint32_t>(0), 22); // em32SetDmVal

		// Flash grenades don't instakill las plagas wolves
		pattern = hook::pattern("74 ? 8A 8E 2E 03 00 00 80 F9 17 74");
		Patch(pattern.count(1).get(0).get<uint32_t>(0), { 0xEB }); // em22SetDmVal, jb -> jmp

		// Restore GameCube-NTSC item drop rates
		{
			// RandomItemCk patches

			// hook GetBulletPoint to use the old GetBulletPoint calculation
			auto pattern = hook::pattern("F7 F9 8A DA E8 ? ? ? ? 89");
			InjectHook(injector::GetBranchDestination(pattern.count(1).get(0).get<uint32_t>(4)).as_int(), GetBulletPoint_ntsc, PATCH_JUMP);

			// Gold chance is always 20% (UHD is 17% in Chapter 1, 20% afterwards)
			pattern = hook::pattern("B1 11 3A D9 0F");
			injector::WriteMemory(pattern.count(1).get(0).get<uint8_t>(1), uint8_t(20), true);

			// Restore the old item drop rate formula:
			// if ( roll < 60 || (No_drop_cnt > 2 && bio4::Rnd() % 10u >= 5) || No_drop_cnt > 5 || (ctrl_flag & 1) != 0 )
			{
				static uint32_t* No_drop_cnt = *hook::pattern("C7 ? ? ? ? ? ? ? ? ? 80 BE C8 4F 00 00 01").count(1).get(0).get<uint32_t*>(2);

				pattern = hook::pattern("F7 46 54 00 10 00 80 ");
				struct RandomItemCk_oldItemDropFormula
				{
					void operator()(injector::reg_pack& regs)
					{
						uint8_t roll = LOBYTE(regs.ebx); // bl register

						// UHD has a dynamic base rate formula based on BulletPoint value and the current chapter 
						// GC instead has a flat % base rate, and then caps ammo at a BulletPoint of 150 later in the func
						if (roll < 60 || (*No_drop_cnt > 2 && bio4::Rnd() % 10 > 4))
							regs.ef |= (1 << regs.carry_flag);
						else
							regs.ef &= ~(1 << regs.carry_flag); // jnb to No_drop_cnt and ctrl_flag checks
					}
				}; injector::MakeInline<RandomItemCk_oldItemDropFormula>(pattern.count(1).get(0).get<uint32_t>(0), pattern.count(1).get(0).get<uint32_t>(110));

				// GC only guarantees a spawn attempt when No_drop_cnt > 5, UHD also tries to spawn an item if No_drop_cnt > 3 and BulletPoint < 80
				pattern = hook::pattern("83 FF 50 73 ? 83");
				Patch(pattern.count(1).get(0).get<uint32_t>(0), { 0x90, 0x90, 0x90, 0xEB }); // jnb -> jmp
			}

			// GC has no special ammo drop func for the r11C cabin fight
			pattern = hook::pattern("75 ? 53 57 E8 ? ? ? ? 83 C4 08 85");
			Patch(pattern.count(1).get(0).get<uint32_t>(0), { 0xEB }); // jnb -> jmp

			// GC has slightly different dynamicDifficultyLevel thresholds for recovery item assist
			pattern = hook::pattern("B9 01 00 00 00 3C 04 73 ? B9 03 00 00 00 3C");
			struct RandomItemCk_oldDynamicDifficulty
			{
				void operator()(injector::reg_pack& regs)
				{
					uint32_t recoveryAssistLimit = 1;

					if (GlobalPtr()->dynamicDifficultyLevel_4F98 < 3)
						recoveryAssistLimit = 3;
					else if (GlobalPtr()->dynamicDifficultyLevel_4F98 > 7)
						recoveryAssistLimit = 0;

					regs.ecx = recoveryAssistLimit;
				}
			}; injector::MakeInline<RandomItemCk_oldDynamicDifficulty>(pattern.count(1).get(0).get<uint32_t>(0), pattern.count(1).get(0).get<uint32_t>(29));

			// GC has no second call to RandomItemCk in case of ctrl_flag & 1
			pattern = hook::pattern("E8 ? ? ? ? 83 C4 10 83 F8 01 0F ? ? ? ? ? E8");
			injector::MakeNOP(pattern.count(1).get(0).get<uint32_t>(0), 17);

			// Skip UHD's 1% green herb chance code (looks like this always gets skipped regardless)
			pattern = hook::pattern("80 B8 7C 84 00 00 06 72");
			Patch(pattern.count(1).get(0).get<uint32_t>(7), { 0xEB }); // jb -> jmp

			// hook GetDropBullet with a reimplementation of the GC GetDropBullet code
			pattern = hook::pattern("53 57 E8 ? ? ? ? 83 C4 08 85 C0 0F ? ? ? ? ? 8B ? ? ? ? ? 8B 42 54");
			ReadCall(injector::GetBranchDestination(pattern.count(1).get(0).get<uint8_t>(2)).as_int(), GetDropBullet_orig);
			InjectHook(injector::GetBranchDestination(pattern.count(1).get(0).get<uint32_t>(2)).as_int(), GetDropBullet_ntsc, PATCH_JUMP);

			// UHD has an extra fallthrough case to drop gold if no ammo or recovery item was generated that we need to get rid of

			// first, capture em_id and ctrl_flag somehow
			pattern = hook::pattern("57 56 6A 10 E8 ? ? ? ? 83 C4 10 83");
			ReadCall(injector::GetBranchDestination(pattern.count(1).get(0).get<uint8_t>(4)).as_int(), RandomItemCk);
			InjectHook(injector::GetBranchDestination(pattern.count(1).get(0).get<uint32_t>(4)).as_int(), RandomItemCk_hook);

			// then use the check for mercs mode to exit the function early if em_id isn't a novistadore and ctrl_flag isn't 1
			pattern = hook::pattern("85 C0 0F ? ? ? ? ? A9 00 00 00 40 0F");
			struct RandomItemCk_skipGoldFallthrough
			{
				void operator()(injector::reg_pack& regs)
				{
					bool isMercenaries = FlagIsSet(GlobalPtr()->Flags_SYSTEM_0_54, uint32_t(Flags_SYSTEM::SYS_OMAKE_ETC_GAME));

					if (isMercenaries || (RandomItemCk_em_id != 0x2D && (RandomItemCk_ctrl_flag & 1) != 1))
						regs.ef &= ~(1 << regs.zero_flag);
					else
						regs.ef |= (1 << regs.zero_flag);
				}
			}; injector::MakeInline<RandomItemCk_skipGoldFallthrough>(pattern.count(1).get(0).get<uint32_t>(8), pattern.count(1).get(0).get<uint32_t>(13));
		}
	}
}


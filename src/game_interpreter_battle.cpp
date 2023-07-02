/*
 * This file is part of EasyRPG Player.
 *
 * EasyRPG Player is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * EasyRPG Player is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with EasyRPG Player. If not, see <http://www.gnu.org/licenses/>.
 */

// Headers
#include "game_actors.h"
#include "game_battle.h"
#include "game_enemyparty.h"
#include "game_interpreter_battle.h"
#include "game_party.h"
#include "game_switches.h"
#include "game_system.h"
#include "game_variables.h"
#include <lcf/reader_util.h>
#include "output.h"
#include "player.h"
#include "game_map.h"
#include "spriteset_battle.h"
#include <cassert>
#include <scene_battle.h>
#include <game_maniacs.h>

enum BranchBattleSubcommand {
	eOptionBranchBattleElse = 1
};

Game_Interpreter_Battle::Game_Interpreter_Battle(Span<const lcf::rpg::TroopPage> pages)
	: Game_Interpreter(true), pages(pages), executed(pages.size(), false)
{
}

bool Game_Interpreter_Battle::AreConditionsMet(const lcf::rpg::TroopPageCondition& condition, Game_Battler* source) {
	if (!condition.flags.switch_a &&
		!condition.flags.switch_b &&
		!condition.flags.variable &&
		!condition.flags.turn &&
		!condition.flags.turn_enemy &&
		!condition.flags.turn_actor &&
		!condition.flags.fatigue &&
		!condition.flags.enemy_hp &&
		!condition.flags.actor_hp &&
		!condition.flags.command_actor
		) {
		// Pages without trigger are never run
		return false;
	}

	if (condition.flags.switch_a && !Main_Data::game_switches->Get(condition.switch_a_id))
		return false;

	if (condition.flags.switch_b && !Main_Data::game_switches->Get(condition.switch_b_id))
		return false;

	if (condition.flags.variable && !(Main_Data::game_variables->Get(condition.variable_id) >= condition.variable_value))
		return false;

	if (condition.flags.turn && !Game_Battle::CheckTurns(Game_Battle::GetTurn(), condition.turn_b, condition.turn_a))
		return false;

	if (condition.flags.turn_enemy) {
		const auto& enemy = (*Main_Data::game_enemyparty)[condition.turn_enemy_id];
		if (source && source != &enemy)
			return false;
		if (!Game_Battle::CheckTurns(enemy.GetBattleTurn(), condition.turn_enemy_b, condition.turn_enemy_a))
			return false;
	}

	if (condition.flags.turn_actor) {
		const auto* actor = Main_Data::game_actors->GetActor(condition.turn_actor_id);
		if (source && source != actor)
			return false;
		if (!Game_Battle::CheckTurns(actor->GetBattleTurn(), condition.turn_actor_b, condition.turn_actor_a))
			return false;
	}

	if (condition.flags.fatigue) {
		int fatigue = Main_Data::game_party->GetFatigue();
		if (fatigue < condition.fatigue_min || fatigue > condition.fatigue_max)
			return false;
	}

	if (condition.flags.enemy_hp) {
		Game_Battler& enemy = (*Main_Data::game_enemyparty)[condition.enemy_id];
		int result = 100 * enemy.GetHp() / enemy.GetMaxHp();
		if (result < condition.enemy_hp_min || result > condition.enemy_hp_max)
			return false;
	}

	if (condition.flags.actor_hp) {
		Game_Actor* actor = Main_Data::game_actors->GetActor(condition.actor_id);
		int result = 100 * actor->GetHp() / actor->GetMaxHp();
		if (result < condition.actor_hp_min || result > condition.actor_hp_max)
			return false;
	}

	if (condition.flags.command_actor) {
		if (!source)
			return false;
		const auto* actor = Main_Data::game_actors->GetActor(condition.command_actor_id);
		if (source != actor)
			return false;
		if (condition.command_id != actor->GetLastBattleAction())
			return false;
	}

	return true;
}

int Game_Interpreter_Battle::ScheduleNextPage(Game_Battler* source) {
	lcf::rpg::TroopPageCondition::Flags f;
	for (auto& ff: f.flags) ff = true;

	return ScheduleNextPage(f, source);
}

static bool HasRequiredCondition(lcf::rpg::TroopPageCondition::Flags page, lcf::rpg::TroopPageCondition::Flags required) {
	for (size_t i = 0; i < page.flags.size(); ++i) {
		if (required.flags[i] && page.flags[i]) {
			return true;
		}
	}
	return false;
}

int Game_Interpreter_Battle::ScheduleNextPage(lcf::rpg::TroopPageCondition::Flags required_conditions, Game_Battler* source) {
	if (IsRunning()) {
		return 0;
	}

	for (size_t i = 0; i < pages.size(); ++i) {
		auto& page = pages[i];
		if (executed[i]
				|| !HasRequiredCondition(page.condition.flags, required_conditions)
				|| !AreConditionsMet(page.condition, source)) {
			continue;
		}
		Clear();
		Push(page.event_commands, 0);
		executed[i] = true;
		return i + 1;
	}
	return 0;
}

// Execute Command.
bool Game_Interpreter_Battle::ExecuteCommand() {
	auto& frame = GetFrame();
	const auto& com = frame.commands[frame.current_command];

	switch (static_cast<Cmd>(com.code)) {
		case Cmd::CallCommonEvent:
			return CommandCallCommonEvent(com);
		case Cmd::ForceFlee:
			return CommandForceFlee(com);
		case Cmd::EnableCombo:
			return CommandEnableCombo(com);
		case Cmd::ChangeMonsterHP:
			return CommandChangeMonsterHP(com);
		case Cmd::ChangeMonsterMP:
			return CommandChangeMonsterMP(com);
		case Cmd::ChangeMonsterCondition:
			return CommandChangeMonsterCondition(com);
		case Cmd::ShowHiddenMonster:
			return CommandShowHiddenMonster(com);
		case Cmd::ChangeBattleBG:
			return CommandChangeBattleBG(com);
		case Cmd::ShowBattleAnimation_B:
			return CommandShowBattleAnimation(com);
		case Cmd::TerminateBattle:
			return CommandTerminateBattle(com);
		case Cmd::ConditionalBranch_B:
			return CommandConditionalBranchBattle(com);
		case Cmd::ElseBranch_B:
			return CommandElseBranchBattle(com);
		case Cmd::EndBranch_B:
			return CommandEndBranchBattle(com);
		case Cmd::Maniac_ControlBattle:
			return CommandManiacControlBattle(com);
		case Cmd::Maniac_ControlAtbGauge:
			return CommandManiacControlAtbGauge(com);
		case Cmd::Maniac_ChangeBattleCommandEx:
			return CommandManiacChangeBattleCommandEx(com);
		case Cmd::Maniac_GetBattleInfo:
			return CommandManiacGetBattleInfo(com);
		default:
			return Game_Interpreter::ExecuteCommand();
	}
}

// Commands

bool Game_Interpreter_Battle::CommandCallCommonEvent(lcf::rpg::EventCommand const& com) {
	if (!Player::IsRPG2k3Commands()) {
		return true;
	}

	int evt_id = com.parameters[0];

	Game_CommonEvent* common_event = lcf::ReaderUtil::GetElement(Game_Map::GetCommonEvents(), evt_id);
	if (!common_event) {
		Output::Warning("CallCommonEvent: Can't call invalid common event {}", evt_id);
		return true;
	}

	Push(common_event);

	return true;
}

bool Game_Interpreter_Battle::CommandForceFlee(lcf::rpg::EventCommand const& com) {
	if (!Player::IsRPG2k3Commands()) {
		return true;
	}

	bool check = com.parameters[2] == 0;

	switch (com.parameters[0]) {
	case 0:
		if (!check || Game_Battle::GetBattleCondition() != lcf::rpg::System::BattleCondition_pincers) {
			this->force_flee_enabled = true;
		}
	    break;
	case 1:
		if (!check || Game_Battle::GetBattleCondition() != lcf::rpg::System::BattleCondition_surround) {
			int num_escaped = 0;
			for (auto* enemy: Main_Data::game_enemyparty->GetEnemies()) {
				if (enemy->Exists()) {
					enemy->SetHidden(true);
					enemy->SetDeathTimer();
					++num_escaped;
				}
			}
			if (num_escaped) {
				Main_Data::game_system->SePlay(Main_Data::game_system->GetSystemSE(Game_System::SFX_Escape));
			}
		}
	    break;
	case 2:
		if (!check || Game_Battle::GetBattleCondition() != lcf::rpg::System::BattleCondition_surround) {
			auto* enemy = Main_Data::game_enemyparty->GetEnemy(com.parameters[1]);
			if (enemy->Exists()) {
				enemy->SetHidden(true);
				enemy->SetDeathTimer();
				Main_Data::game_system->SePlay(Main_Data::game_system->GetSystemSE(Game_System::SFX_Escape));
			}
		}
	    break;
	}

	return true;
}

bool Game_Interpreter_Battle::CommandEnableCombo(lcf::rpg::EventCommand const& com) {
	if (!Player::IsRPG2k3Commands()) {
		return true;
	}

	int actor_id = com.parameters[0];

	if (!Main_Data::game_party->IsActorInParty(actor_id)) {
		return true;
	}

	int command_id = com.parameters[1];
	int multiple = com.parameters[2];

	Game_Actor* actor = Main_Data::game_actors->GetActor(actor_id);

	if (!actor) {
		Output::Warning("EnableCombo: Invalid actor ID {}", actor_id);
		return true;
	}

	actor->SetBattleCombo(command_id, multiple);

	return true;
}

bool Game_Interpreter_Battle::CommandChangeMonsterHP(lcf::rpg::EventCommand const& com) {
	int id = com.parameters[0];
	Game_Enemy& enemy = (*Main_Data::game_enemyparty)[id];
	bool lose = com.parameters[1] > 0;
	bool lethal = com.parameters[4] > 0;
	int hp = enemy.GetHp();

	if (enemy.IsDead())
		return true;

	int change = 0;
	switch (com.parameters[2]) {
	case 0:
		change = com.parameters[3];
	    break;
	case 1:
		change = Main_Data::game_variables->Get(com.parameters[3]);
	    break;
	case 2:
		change = com.parameters[3] * hp / 100;
	    break;
	}

	if (lose) {
		change = -change;
	}

	enemy.ChangeHp(change, lethal);

	auto& scene = Scene::instance;
	if (scene) {
		scene->OnEventHpChanged(&enemy, change);
	}

	if (enemy.IsDead()) {
		Main_Data::game_system->SePlay(Main_Data::game_system->GetSystemSE(Main_Data::game_system->SFX_EnemyKill));
		enemy.SetDeathTimer();
	}

	return true;
}

bool Game_Interpreter_Battle::CommandChangeMonsterMP(lcf::rpg::EventCommand const& com) {
	int id = com.parameters[0];
	Game_Enemy& enemy = (*Main_Data::game_enemyparty)[id];
	bool lose = com.parameters[1] > 0;
	int sp = enemy.GetSp();

	int change = 0;
	switch (com.parameters[2]) {
	case 0:
		change = com.parameters[3];
	    break;
	case 1:
		change = Main_Data::game_variables->Get(com.parameters[3]);
	    break;
	}

	if (lose)
		change = -change;

	sp += change;

	enemy.SetSp(sp);

	return true;
}

bool Game_Interpreter_Battle::CommandChangeMonsterCondition(lcf::rpg::EventCommand const& com) {
	Game_Enemy& enemy = (*Main_Data::game_enemyparty)[com.parameters[0]];
	bool remove = com.parameters[1] > 0;
	int state_id = com.parameters[2];
	if (remove) {
		// RPG_RT BUG: Monster dissapears immediately and doesn't animate death
		enemy.RemoveState(state_id, false);
	} else {
		enemy.AddState(state_id, true);
	}
	return true;
}

bool Game_Interpreter_Battle::CommandShowHiddenMonster(lcf::rpg::EventCommand const& com) {
	Game_Enemy& enemy = (*Main_Data::game_enemyparty)[com.parameters[0]];
	enemy.SetHidden(false);
	return true;
}

bool Game_Interpreter_Battle::CommandChangeBattleBG(lcf::rpg::EventCommand const& com) {
	Game_Battle::ChangeBackground(ToString(com.string));
	return true;
}

bool Game_Interpreter_Battle::CommandShowBattleAnimation(lcf::rpg::EventCommand const& com) {
	int animation_id = com.parameters[0];
	int target = com.parameters[1];
	bool waiting_battle_anim = com.parameters[2] != 0;
	bool allies = false;

	if (Player::IsRPG2k3()) {
		allies = com.parameters[3] != 0;
	}

	int frames = 0;

	if (target < 0) {
		std::vector<Game_Battler*> v;

		if (allies) {
			Main_Data::game_party->GetBattlers(v);
		} else {
			Main_Data::game_enemyparty->GetBattlers(v);
		}
		auto iter = std::remove_if(v.begin(), v.end(),
				[](auto* target) { return !(target->Exists() || (target->GetType() == Game_Battler::Type_Ally && target->IsDead())); });
		v.erase(iter, v.end());

		frames = Game_Battle::ShowBattleAnimation(animation_id, v, false);
	}
	else {
		Game_Battler* battler_target = nullptr;

		if (allies) {
			// Allies counted from 1
			target -= 1;
			if (target >= 0 && target < Main_Data::game_party->GetBattlerCount()) {
				battler_target = &(*Main_Data::game_party)[target];
			}
		}
		else {
			if (target < Main_Data::game_enemyparty->GetBattlerCount()) {
				battler_target = &(*Main_Data::game_enemyparty)[target];
			}
		}

		if (battler_target) {
			frames = Game_Battle::ShowBattleAnimation(animation_id, { battler_target });
		}
	}

	if (waiting_battle_anim) {
		_state.wait_time = frames;
	}

	return true;
}

bool Game_Interpreter_Battle::CommandTerminateBattle(lcf::rpg::EventCommand const& /* com */) {
	_async_op = AsyncOp::MakeTerminateBattle(static_cast<int>(BattleResult::Abort));
	return false;
}

// Conditional branch.
bool Game_Interpreter_Battle::CommandConditionalBranchBattle(lcf::rpg::EventCommand const& com) {
	bool result = false;
	int value1, value2;

	switch (com.parameters[0]) {
		case 0:
			// Switch
			result = Main_Data::game_switches->Get(com.parameters[1]) == (com.parameters[2] == 0);
			break;
		case 1:
			// Variable
			value1 = Main_Data::game_variables->Get(com.parameters[1]);
			if (com.parameters[2] == 0) {
				value2 = com.parameters[3];
			} else {
				value2 = Main_Data::game_variables->Get(com.parameters[3]);
			}
			switch (com.parameters[4]) {
				case 0:
					// Equal to
					result = (value1 == value2);
					break;
				case 1:
					// Greater than or equal
					result = (value1 >= value2);
					break;
				case 2:
					// Less than or equal
					result = (value1 <= value2);
					break;
				case 3:
					// Greater than
					result = (value1 > value2);
					break;
				case 4:
					// Less than
					result = (value1 < value2);
					break;
				case 5:
					// Different
					result = (value1 != value2);
					break;
			}
			break;
		case 2: {
			// Hero can act
			Game_Actor* actor = Main_Data::game_actors->GetActor(com.parameters[1]);

			if (!actor) {
				Output::Warning("ConditionalBranchBattle: Invalid actor ID {}", com.parameters[1]);
				// Use Else Branch
				SetSubcommandIndex(com.indent, 1);
				SkipToNextConditional({Cmd::ElseBranch_B, Cmd::EndBranch_B}, com.indent);
				return true;
			}

			result = actor->CanAct();
			break;
		}
		case 3:
			// Monster can act
			if (com.parameters[1] < Main_Data::game_enemyparty->GetBattlerCount()) {
				result = (*Main_Data::game_enemyparty)[com.parameters[1]].CanAct();
			}
			break;
		case 4:
			// Monster is the current target
			result = (targets_single_enemy && target_enemy_index == com.parameters[1]);
			break;
		case 5: {
			// Hero uses the ... command
			if (current_actor_id == com.parameters[1]) {
				auto *actor = Main_Data::game_actors->GetActor(current_actor_id);
				if (actor) {
					result = actor->GetLastBattleAction() == com.parameters[2];
				}
			}
			break;
		}
	}

	int sub_idx = subcommand_sentinel;
	if (!result) {
		sub_idx = eOptionBranchBattleElse;
		SkipToNextConditional({Cmd::ElseBranch_B, Cmd::EndBranch_B}, com.indent);
	}

	SetSubcommandIndex(com.indent, sub_idx);
	return true;
}

bool Game_Interpreter_Battle::CommandElseBranchBattle(lcf::rpg::EventCommand const& com) { //code 23310
	return CommandOptionGeneric(com, eOptionBranchBattleElse, {Cmd::EndBranch_B});
}

bool Game_Interpreter_Battle::CommandEndBranchBattle(lcf::rpg::EventCommand const& /* com */) { //code 23311
	return true;
}

bool Game_Interpreter_Battle::CommandManiacControlBattle(lcf::rpg::EventCommand const& com) {
	if (!Player::IsPatchManiac()) {
		return true;
	}
	if (com.parameters[0] == 0) {
		// Target

		ManiacsBattle::Set_ATBCE(com.parameters[2]);
		ManiacsBattle::Set_ATBVar(com.parameters[3]);
	}
	else if (com.parameters[0] == 1) {
		// Damage Pop

		ManiacsBattle::Set_DamageCE(com.parameters[2]);
		ManiacsBattle::Set_DamageVar(com.parameters[3]);
	}
	else if (com.parameters[0] == 2) {
		// Target

		ManiacsBattle::Set_TargetCE(com.parameters[2]);
		ManiacsBattle::Set_TargetVar(com.parameters[3]);
	}
	else if (com.parameters[0] == 3) {
		// Set State

		ManiacsBattle::Set_StateCE(com.parameters[2]);
		ManiacsBattle::Set_StateVar(com.parameters[3]);
	}
	else if (com.parameters[0] == 4) {
		// Set Stats

		ManiacsBattle::Set_StatsCE(com.parameters[2]);
		ManiacsBattle::Set_StatsVar(com.parameters[3]);
	}
	else
		Output::Warning("Maniac Patch: Command ControlBattle not supported {}", com.parameters[0]);
	return true;
}

bool Game_Interpreter_Battle::CommandManiacControlAtbGauge(lcf::rpg::EventCommand const& com) {
	if (!Player::IsPatchManiac()) {
		return true;
	}

	//Output::Warning("Maniac Patch: {}-{}-{}-{}-{}-{}-{}", com.parameters[0], com.parameters[1], com.parameters[2], com.parameters[3], com.parameters[4], com.parameters[5], com.parameters[6]);

	int user_type = com.parameters[0];
	int user_var = com.parameters[1];
	int user_id = com.parameters[2];

	int operation = com.parameters[3];
	int operand = com.parameters[4];

	int incr_type = com.parameters[5];
	int incr_value = com.parameters[6];

	//Output::Debug(std::to_string(user_type) + "/" + std::to_string(user_id) + "/" + std::to_string(user_var) + "/" + std::to_string(type) + "/" + std::to_string(var_id));

	Game_Battler* battler = NULL;
	Game_Party_Base* party = NULL;
	int id;
	int incr;

	if (user_type == 0) {
		if (user_var == 1)
			id = Main_Data::game_variables.get()->Get(user_id);
		else if (user_var == 2)
			id = Main_Data::game_variables.get()->Get(Main_Data::game_variables.get()->Get(user_id));
		else
			id = user_id;

		battler = Main_Data::game_actors.get()->GetActor(id);
	}
	else if (user_type == 1) {
		if (user_var == 1)
			id = Main_Data::game_variables.get()->Get(user_id);
		else if (user_var == 2)
			id = Main_Data::game_variables.get()->Get(Main_Data::game_variables.get()->Get(user_id));
		else
			id = user_id;
		battler = Main_Data::game_party.get()->GetActor(id);

	}
	else if (user_type == 2) {

		party = Main_Data::game_party.get();
	}
	else if (user_type == 3) {

		if (user_var == 1)
			id = Main_Data::game_variables.get()->Get(user_id);
		else if (user_var == 2)
			id = Main_Data::game_variables.get()->Get(Main_Data::game_variables.get()->Get(user_id));
		else
			id = user_id;

		battler = Main_Data::game_enemyparty.get()->GetEnemy(id);

	}
	else if (user_type == 4) {

		party = Main_Data::game_enemyparty.get();
	}

	if (incr_type == 1)
		incr = Main_Data::game_variables.get()->Get(incr_value);
	else if (incr_type == 2)
		incr = Main_Data::game_variables.get()->Get(Main_Data::game_variables.get()->Get(incr_value));
	else
		incr = incr_value;



	if (user_type == 2 || user_type == 4) {
		std::vector<Game_Battler*> o;
		party->GetBattlers(o);

		for (int i = 0; i < o.size(); i++) {
			Game_Battler* b = o[i];
			int c = incr;
			if (operand == 1)
				c = incr * 300000 / 100;

			if (operation == 0)
				b->SetAtbGauge(c);
			else if (operation == 1)
				b->IncrementAtbGauge(c);
			else if (operation == 2)
				b->IncrementAtbGauge(-c);

		}
	}
	else if (user_type == 0 || user_type == 1 || user_type == 3) {

		if (operand == 1)
			incr = incr * 300000 / 100;

		if (operation == 0)
			battler->SetAtbGauge(incr);
		else if (operation == 1)
			battler->IncrementAtbGauge(incr);
		else if (operation == 2)
			battler->IncrementAtbGauge(-incr);
	}


	return true;
}

bool Game_Interpreter_Battle::CommandManiacChangeBattleCommandEx(lcf::rpg::EventCommand const& com) {
	if (!Player::IsPatchManiac()) {
		return true;
	}
	if (com.parameters[0] == 0)
		lcf::Data::battlecommands.easyrpg_enable_battle_row_command = true;
	else
		lcf::Data::battlecommands.easyrpg_enable_battle_row_command = false;

	//Output::Debug("CommandEx : {}-{}-{}-{}-{}", com.parameters[0], com.parameters[1], com.parameters[2], com.parameters[3], com.parameters[4], com.parameters[5]);

	int i1 = (com.parameters[1]) % 2;
	int i2 = (com.parameters[1] >> 1) % 2;
	int i3 = (com.parameters[1] >> 2) % 2;
	int i4 = (com.parameters[1] >> 3) % 2;
	int i5 = (com.parameters[1] >> 4) % 2;

	std::vector<int16_t> cmds = {};

	if (i1 == 0)
		cmds.push_back(0);

	if (i2 == 0)
		cmds.push_back(1);

	if (i3 == 0)
		cmds.push_back(2);

	if (i4 == 1)
		cmds.push_back(3);

	if (i5 == 1)
		cmds.push_back(4);

	Scene_Battle* scene = (Scene_Battle*)Scene::Find(Scene::Battle).get();
	if (scene)
	{
		if (scene->GetOptionsWindow()) {
			scene->reset_easyrpg_battle_options(cmds);

		}
	}

	return true;
}

bool Game_Interpreter_Battle::CommandManiacGetBattleInfo(lcf::rpg::EventCommand const& com) {
	if (!Player::IsPatchManiac()) {
		return true;
	}
	int user_type = com.parameters[0];
	int user_id = com.parameters[3];
	int user_var = com.parameters[2];
	int type = com.parameters[1];
	int var_id = com.parameters[4];

	//Output::Debug(std::to_string(user_type) + "/" + std::to_string(user_id) + "/" + std::to_string(user_var) + "/" + std::to_string(type) + "/" + std::to_string(var_id));

	Game_Battler* battler = NULL;
	Game_Party_Base* party = NULL;
	int id;

	if (user_type == 0) {
		if (user_var == 1)
			id = Main_Data::game_variables.get()->Get(user_id);
		else if (user_var == 2)
			id = Main_Data::game_variables.get()->Get(Main_Data::game_variables.get()->Get(user_id));
		else
			id = user_id;

		battler = Main_Data::game_actors.get()->GetActor(id);
	}
	else if (user_type == 1) {
		if (user_var == 1)
			id = Main_Data::game_variables.get()->Get(user_id);
		else if (user_var == 2)
			id = Main_Data::game_variables.get()->Get(Main_Data::game_variables.get()->Get(user_id));
		else
			id = user_id;
		battler = Main_Data::game_party.get()->GetActor(id);

	}
	else if (user_type == 2) {

		party = Main_Data::game_party.get();
	}
	else if (user_type == 3) {

		if (user_var == 1)
			id = Main_Data::game_variables.get()->Get(user_id);
		else if (user_var == 2)
			id = Main_Data::game_variables.get()->Get(Main_Data::game_variables.get()->Get(user_id));
		else
			id = user_id;

		battler = Main_Data::game_enemyparty.get()->GetEnemy(id);

	}
	else if (user_type == 4) {

		party = Main_Data::game_enemyparty.get();
	}

	if (type == 0) {
		if (user_type == 2 || user_type == 4) {
			// Party
			std::vector<Game_Battler*> o;
			party->GetBattlers(o);
			Main_Data::game_variables->Set(var_id, o.size());
			for (int i = 0; i < o.size(); i++) {
				Main_Data::game_variables->Set(var_id + i + 1, i);
			}
		}
		else if (user_type == 0 || user_type == 1 || user_type == 3) {
			// Stats
			for (int i = 0; i < 4; ++i) {
				Main_Data::game_variables->Set(var_id + i, 0);
			}

			if (battler) {

				int stat = battler->GetAtk() - battler->GetBaseAtk();
				Main_Data::game_variables->Set(var_id, stat);

				stat = battler->GetDef() - battler->GetBaseDef();
				Main_Data::game_variables->Set(var_id + 1, stat);

				stat = battler->GetSpi() - battler->GetBaseSpi();
				Main_Data::game_variables->Set(var_id + 2, stat);

				stat = battler->GetAgi() - battler->GetBaseAgi();
				Main_Data::game_variables->Set(var_id + 3, stat);

			}
		}
	}
	else if (type == 1) {
		if (user_type == 2 || user_type == 4) {
			// Party alive
			std::vector<Game_Battler*> o;
			party->GetBattlers(o);

			for (int i = 0; i < o.size(); i++) {
				Main_Data::game_variables->Set(var_id + i, 0);
			}

			o.clear();

			party->GetActiveBattlers(o);
			Main_Data::game_variables->Set(var_id, o.size());
			for (int i = 0; i < o.size(); i++) {
				Main_Data::game_variables->Set(var_id + i + 1, i);
			}
		}
		else if (user_type == 0 || user_type == 1 || user_type == 3) {

			// Status

			int m = lcf::Data::states.size();
			for (int i = 0; i < m; ++i) {
				Main_Data::game_variables->Set(var_id + i, 0);
			}
			Main_Data::game_variables->Set(var_id, m);

			if (battler) {
				std::vector<int16_t> states = battler->GetStates();

				for (int i = 0; i < states.size(); ++i) {
					Main_Data::game_variables->Set(var_id + i + 1, states[i]);
				}
			}
		}
	}
	else if (type == 2) {
		if (user_type == 2 || user_type == 4) {
			// Party can move
			std::vector<Game_Battler*> o;
			party->GetBattlers(o);

			int size = 0;
			for (int i = 0; i < o.size(); i++) {
				Main_Data::game_variables->Set(var_id + i + 1, 0);
				if (o[i]->CanAct() && !o[i]->IsHidden()) {
					Main_Data::game_variables->Set(var_id + size + 1, i);
					size += 1;
				}
			}

			Main_Data::game_variables->Set(var_id, size);
		}
		else if (user_type == 0 || user_type == 1 || user_type == 3) {

			// Attributes

			int m = lcf::Data::attributes.size();
			for (int i = 0; i < m; ++i) {
				Main_Data::game_variables->Set(var_id, 0);
			}
			Main_Data::game_variables->Set(var_id, m);
			if (battler) {
				for (int i = 0; i < m; ++i) {
					Main_Data::game_variables->Set(var_id + i + 1, battler->GetAttributeRate(i));
				}
			}
		}
	}
	else if (type == 3) {
		if (user_type == 0 || user_type == 1 || user_type == 3) {
			// Other
			int m = 6;
			for (int i = 0; i < m; ++i) {
				Main_Data::game_variables->Set(var_id + i, 0);
			}

			if (battler) {

				Main_Data::game_variables->Set(var_id, battler->GetBattlePosition().x);
				Main_Data::game_variables->Set(var_id + 1, battler->GetBattlePosition().y + 8);

				if (battler->CanAct())
					Main_Data::game_variables->Set(var_id + 2, 1);
				if (battler->IsDefending())
					Main_Data::game_variables->Set(var_id + 3, 1);
				if (battler->IsCharged())
					Main_Data::game_variables->Set(var_id + 4, 1);
				if (battler->IsHidden())
					Main_Data::game_variables->Set(var_id + 5, 1);
			}
		}
	}

	return true;
}


//TODO
Game_CommonEvent* Game_Interpreter_Battle::StartCommonEvent(int i) {
	int evt_id = i;

	Game_CommonEvent* common_event = lcf::ReaderUtil::GetElement(Game_Map::GetCommonEvents(), evt_id);
	if (!common_event) {
		Output::Warning("CallCommonEvent: Can't call invalid common event {}", evt_id);
		return common_event;
	}
	bool b = false;
	for (int j = 0; j < _state.stack.size(); j++) {

		//Output::Debug("S {} C {}", _state.stack[j].event_id, i);
		if (_state.stack[j].event_id == -i) {
			b = true;
			//common_event = _state.stack[j];
		}
	}

	if (!b)
		PushCommonEvent(common_event);
	else {
		//common_event->Update(false);
		return NULL;
	}

	return common_event;
}

void Game_Interpreter_Battle::PushCommonEvent(Game_CommonEvent* ev) {
	Push(ev->GetList(), -ev->GetIndex(), false);
	//Push(ev->GetList(), 0, false);
}

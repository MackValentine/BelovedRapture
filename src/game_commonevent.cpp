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
#include "game_commonevent.h"
#include "game_map.h"
#include "game_switches.h"
#include "game_interpreter_map.h"
#include "main_data.h"
#include <lcf/reader_util.h>
#include <cassert>
#include <game_maniacs.h>
#include <output.h>

Game_CommonEvent::Game_CommonEvent(int common_event_id) :
	common_event_id(common_event_id)
{
	auto* ce = lcf::ReaderUtil::GetElement(lcf::Data::commonevents, common_event_id);

	if ((ce->trigger == lcf::rpg::EventPage::Trigger_parallel)// || ce->trigger == 7 || ce->trigger == 6)
			&& !ce->event_commands.empty()) {
		interpreter.reset(new Game_Interpreter_Map());
		interpreter->Push(this);
	}


}

void Game_CommonEvent::ForceCreate(int ce_ID) {
	auto* ce = lcf::ReaderUtil::GetElement(lcf::Data::commonevents, ce_ID);

	if ((ce->trigger == lcf::rpg::EventPage::Trigger_parallel || ce->trigger == 7 || ce->trigger == 6 || ce->ID == ManiacsBattle::Get_ATBCE() || ce->ID == ManiacsBattle::Get_TargetCE() ||
		ce->ID == ManiacsBattle::Get_DamageCE() || ce->ID == ManiacsBattle::Get_StateCE() || ce->ID == ManiacsBattle::Get_StatsCE())
		&& !ce->event_commands.empty())
	{
		bool main_flag = false;
	/*	main_flag = true;
		if (ce->trigger != 7)
			main_flag = false;*/

		//interpreter.reset(new Game_Interpreter_Map(main_flag));
		interpreter_pp.reset(new Game_Interpreter_Battle(main_flag));
		interpreter_pp->Push(this);

	}
}

void Game_CommonEvent::SetSaveData(const lcf::rpg::SaveEventExecState& data) {
	// RPG_RT Savegames have empty stacks for parallel events.
	// We are LSD compatible but don't load these into interpreter.
	if (!data.stack.empty() && !data.stack.front().commands.empty()) {
		if (!interpreter) {
			interpreter.reset(new Game_Interpreter_Map());
		}
		interpreter->SetState(data);
	}
}

AsyncOp Game_CommonEvent::Update(bool resume_async) {
	if (interpreter && IsWaitingBackgroundExecution(resume_async)) {
		assert(interpreter->IsRunning());
		interpreter->Update(!resume_async);

		// Suspend due to async op ...
		if (interpreter->IsAsyncPending()) {
			return interpreter->GetAsyncOp();
		}
	}

	return {};
}

void Game_CommonEvent::KillCE() {
	if (interpreter_pp) {
		interpreter_pp->Clear();
		interpreter_pp.reset(new Game_Interpreter_Battle(true));
	}
}

AsyncOp Game_CommonEvent::UpdateBattle(bool resume_async, int ce_ID) {
	if (interpreter_pp) {
		if (interpreter_pp->IsRunning()) {
			interpreter_pp->Update(!resume_async);

			// Suspend due to async op ...
			if (interpreter_pp->IsAsyncPending()) {
				return interpreter_pp->GetAsyncOp();
			}
		}
		else {
			Output::Warning("Battle Interpreter not running");
		}
	}
	else {
		Output::Warning("Common Event {} is empty or doesn't exist", ce_ID);
	}
	return {};
}
int Game_CommonEvent::GetId() const {
	return common_event_id;
}

int Game_CommonEvent::GetIndex() const {
	return common_event_id;
}

// Game_Map ensures validity of Common Events

StringView Game_CommonEvent::GetName() const {
	return lcf::ReaderUtil::GetElement(lcf::Data::commonevents, common_event_id)->name;
}

bool Game_CommonEvent::GetSwitchFlag() const {
	return lcf::ReaderUtil::GetElement(lcf::Data::commonevents, common_event_id)->switch_flag;
}

int Game_CommonEvent::GetSwitchId() const {
	return lcf::ReaderUtil::GetElement(lcf::Data::commonevents, common_event_id)->switch_id;
}

int Game_CommonEvent::GetTrigger() const {
	return lcf::ReaderUtil::GetElement(lcf::Data::commonevents, common_event_id)->trigger;
}

std::vector<lcf::rpg::EventCommand>& Game_CommonEvent::GetList() {
	return lcf::ReaderUtil::GetElement(lcf::Data::commonevents, common_event_id)->event_commands;
}

lcf::rpg::SaveEventExecState Game_CommonEvent::GetSaveData() {
	lcf::rpg::SaveEventExecState state;
	if (interpreter) {
		state = interpreter->GetSaveState();
	}
	if (GetTrigger() == lcf::rpg::EventPage::Trigger_parallel && state.stack.empty()) {
		// RPG_RT always stores an empty stack frame for parallel events.
		state.stack.push_back({});
	}
	return state;
}

bool Game_CommonEvent::IsWaitingForegroundExecution() const {
	auto* ce = lcf::ReaderUtil::GetElement(lcf::Data::commonevents, common_event_id);
	return ce->trigger == lcf::rpg::EventPage::Trigger_auto_start &&
		(!ce->switch_flag || Main_Data::game_switches->Get(ce->switch_id))
		&& !ce->event_commands.empty();
}

bool Game_CommonEvent::IsWaitingBackgroundExecution(bool force_run) const {
	auto* ce = lcf::ReaderUtil::GetElement(lcf::Data::commonevents, common_event_id);
	return ce->trigger == lcf::rpg::EventPage::Trigger_parallel &&
		(force_run || !ce->switch_flag || Main_Data::game_switches->Get(ce->switch_id));
}

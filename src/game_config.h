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

#ifndef EP_GAME_CONFIG_H
#define EP_GAME_CONFIG_H

#include "config_param.h"
#include "filesystem.h"
#include "options.h"
#include "input_buttons.h"
#include "utils.h"

class CmdlineParser;

enum class ScalingMode {
	/** Nearest neighbour to fit screen */
	Nearest,
	/** Like NN but only scales to integers */
	Integer,
	/** Integer followed by Bilinear downscale to fit screen */
	Bilinear,
};

struct Game_ConfigPlayer {
	StringConfigParam autobattle_algo{ "", "", "" };
	StringConfigParam enemyai_algo{ "", "", "" };

	void Hide();
};

struct Game_ConfigVideo {
	LockedConfigParam<std::string> renderer{ "Renderer", "The rendering engine", "auto" };
	BoolConfigParam vsync{ "V-Sync", "Toggle V-Sync mode (Recommended: ON)", true };
	BoolConfigParam fullscreen{ "Fullscreen", "Toggle between fullscreen and window mode", true };
	BoolConfigParam show_fps{ "Show FPS", "Toggle display of the FPS counter", false };
	BoolConfigParam fps_render_window{ "Show FPS in Window", "Show FPS inside the window when in window mode", false };
	RangeConfigParam<int> fps_limit{ "Frame Limiter", "Toggle the frames per second limit (Recommended: 60)", DEFAULT_FPS, 0, 99999 };
	ConfigParam<int> window_zoom{ "Window Zoom", "Toggle the window zoom level", 2 };
	EnumConfigParam<ScalingMode, 3> scaling_mode{ "Scaling method", "How the output is scaled",
		ScalingMode::Bilinear, Utils::MakeSvArray("Nearest", "Integer", "Bilinear"),
		Utils::MakeSvArray("Scale to screen size (Causes scaling artifacts)", "Scale to multiple of the game resolution", "Like Nearest, but output is blurred to avoid artifacts")};
	BoolConfigParam stretch{ "Stretch", "Stretches the window to screen width", false };
	BoolConfigParam touch_ui{ "Touch Ui", "Display the touch ui", true };

	// These are never shown and are used to restore the window to the previous position
	ConfigParam<int> window_x{ "", "", -1 };
	ConfigParam<int> window_y{ "", "", -1 };
	ConfigParam<int> window_width{ "", "", -1 };
	ConfigParam<int> window_height{ "", "", -1 };

	void Hide();
};

struct Game_ConfigAudio {
	RangeConfigParam<int> music_volume{ "BGM Volume", "Volume of the background music", 100, 0, 100 };
	RangeConfigParam<int> sound_volume{ "SFX Volume", "Volume of the sound effects", 100, 0, 100 };

	void Hide();
};

struct Game_ConfigInput {
	BoolConfigParam gamepad_swap_analog{ "Gamepad: Swap Analog Sticks", "Swap left and right stick", false };
	BoolConfigParam gamepad_swap_dpad_with_buttons{ "Gamepad: Swap D-Pad with buttons", "Swap D-Pad with ABXY-Buttons", false };
	BoolConfigParam gamepad_swap_ab_and_xy{ "Gamepad: Swap AB and XY", "Swap A and B with X and Y", false };
	Input::ButtonMappingArray buttons;

	void Hide();
};

struct Game_Config {
	/** Gameplay subsystem options */
	Game_ConfigPlayer player;

	/** Video subsystem options */
	Game_ConfigVideo video;

	/** Audio subsystem options */
	Game_ConfigAudio audio;

	/** Input subsystem options */
	Game_ConfigInput input;

	/**
	 * Create an application config. This first determines the config file path if any,
	 * loads the config file, then loads command line arguments.
	 */
	static Game_Config Create(CmdlineParser& cp);

	/** @return config file path from command line args if found */
	static std::string GetConfigPath(CmdlineParser& cp);

	/**
	 * Returns the a filesystem view to the global config directory
	 */
	static FilesystemView GetGlobalConfigFilesystem();

	/**
	 * Returns a handle to the global config file for reading.
	 * The file is created if it does not exist.
	 *
	 * @return handle to the global file
	 */
	static Filesystem_Stream::InputStream GetGlobalConfigFileInput();

	/**
	 * Returns a handle to the global config file for writing.
	 * The file is created if it does not exist.
	 *
	 * @return handle to the global file
	 */
	static Filesystem_Stream::OutputStream GetGlobalConfigFileOutput();

	/**
	 * Load configuration values from a stream;
	 *
	 * @param is stream to read from.
	 * @post values of this are updated with values found in the stream.
	 */
	void LoadFromStream(Filesystem_Stream::InputStream& is);

	/**
	 * Load configuration values from a command line arguments.
	 *
	 * @param cp the command line parser to use.
	 * @post values of this are updated with values found in command line args.
	 */
	void LoadFromArgs(CmdlineParser& cp);

	/**
	 * Writes our configuration to the given stream.
	 *
	 * @param os stream to write to
	 */
	void WriteToStream(Filesystem_Stream::OutputStream& os) const;
};

#endif

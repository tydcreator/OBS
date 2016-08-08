/******************************************************************************
	Copyright (C) 2016 by Jo�o Portela <email@joaoportela.net>

	This program is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 2 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program.  If not, see <http://www.gnu.org/licenses/>.
	******************************************************************************/

#include<iostream>
#include<string>
#include<memory>

#include<boost/program_options.hpp>

#include<obs.h>
#include<obs.hpp>
#include<util/platform.h>

#include"enum_types.hpp"
#include"monitor_info.hpp"
#include"setup_obs.hpp"
#include"event_loop.hpp"

namespace {
	namespace Ret {
		enum ENUM : int {
			success = 0,
			error_in_command_line = 1,
			error_unhandled_exception = 2,
			error_obs = 3
		};
	}

	struct CliOptions {
		// default values
		static const std::string default_encoder;
		static const int default_video_bitrate = 2500;

		// cli options
		int monitor_to_record = 0;
		std::string encoder;
		int video_bitrate = 2500;
		std::vector<std::string> outputs_paths;
		int audio_index = -1;
		bool audio_is_output = false;
		bool show_help = false;
		bool list_monitors = false;
		bool list_audios = false;
		bool list_encoders = false;
		bool list_inputs = false;
		bool list_outputs = false;
	} cli_options;
	const std::string CliOptions::default_encoder = "obs_x264";
} // namespace

/**
* Resets/Initializes video settings.
*
*   Calls obs_reset_video internally. Assumes some video options.
*/
void reset_video(int monitor_index) {
	struct obs_video_info ovi;

	ovi.fps_num = 60;
	ovi.fps_den = 1;

	MonitorInfo monitor = monitor_at_index(monitor_index);
	ovi.graphics_module = "libobs-d3d11.dll"; // DL_D3D11
	ovi.base_width = monitor.width;
	ovi.base_height = monitor.height;
	ovi.output_width = monitor.width;
	ovi.output_height = monitor.height;
	ovi.output_format = VIDEO_FORMAT_NV12;
	ovi.colorspace = VIDEO_CS_601;
	ovi.range = VIDEO_RANGE_PARTIAL;
	ovi.adapter = 0;
	ovi.gpu_conversion = true;
	ovi.scale_type = OBS_SCALE_BICUBIC;

	int ret = obs_reset_video(&ovi);
	if (ret != OBS_VIDEO_SUCCESS) {
		std::cout << "reset_video failed!" << std::endl;
	}
}

/**
* Resets/Initializes video settings.
*
*   Calls obs_reset_audio internally. Assumes some audio options.
*/
void reset_audio() {
	struct obs_audio_info ai;
	ai.samples_per_sec = 44100;
	ai.speakers = SPEAKERS_STEREO;

	bool success = obs_reset_audio(&ai);
	if (!success)
		std::cerr << "Audio reset failed!" << std::endl;
}

/**
* Start recording to multiple outputs
*
*   Calls obs_output_start(output) internally.
*/
void start_recording(std::vector<OBSOutput> outputs) {
	int outputs_started = 0;
	for (auto output : outputs) {
		bool success = obs_output_start(output);
		outputs_started += success ? 1 : 0;
	}

	if (outputs_started == outputs.size()) {
		std::cout << "Recording started for all outputs!" << std::endl;
	}
	else {
		size_t outputs_failed = outputs.size() - outputs_started;
		std::cerr << outputs_failed << "/" << outputs.size() << " file outputs are not recording." << std::endl;
	}
}

bool should_print_lists() {
	return cli_options.list_monitors
		|| cli_options.list_audios
		|| cli_options.list_encoders
		|| cli_options.list_inputs
		|| cli_options.list_outputs;
}

bool do_print_lists() {
	if (cli_options.list_monitors) {
		print_monitors_info();
	}
	if (cli_options.list_audios) {
		print_obs_enum_audio_types();
	}
	if (cli_options.list_encoders) {
		print_obs_enum_encoder_types();
	}
	if (cli_options.list_inputs) {
		print_obs_enum_input_types();
	}
	if (cli_options.list_outputs) {
		print_obs_enum_output_types();
	}

	return should_print_lists();
}


int parse_args(int argc, char **argv) {
	namespace po = boost::program_options;

	// Declare the supported options.
	po::options_description desc("obs-cli Allowed options");
	desc.add_options()
		("help,h", "Show help message")
		("listmonitors", "List available monitors resolutions")
		("listinputs", "List available inputs")
		("listencoders", "List available encoders")
		("listoutputs", "List available outputs")
		("listaudios", "List available audios")
		("monitor,m", po::value<int>(&cli_options.monitor_to_record)->required(), "set monitor to be recorded")
		("audio,a", po::value<int>(&cli_options.audio_index), "set audio to be recorded")
		("aisoutput", po::bool_switch(&cli_options.audio_is_output), "set if audio capture is output")
		("encoder,e", po::value<std::string>(&cli_options.encoder)->default_value(CliOptions::default_encoder), "set encoder")
		("vbitrate,v", po::value<int>(&cli_options.video_bitrate)->default_value(CliOptions::default_video_bitrate), "set video bitrate. suggested values: 1200 for low, 2500 for medium, 5000 for high")
		("output,o", po::value<std::vector<std::string>>(&cli_options.outputs_paths)->required(), "set file destination, can be set multiple times for multiple outputs")
		;

	try {
		po::variables_map vm;
		po::store(po::parse_command_line(argc, argv, desc), vm);

		// must be manually set instead of using po::switch because they are
		// called before po::notify
		cli_options.show_help = vm.count("help") > 0;
		cli_options.list_monitors = vm.count("listmonitors") > 0;
		cli_options.list_inputs = vm.count("listinputs") > 0;
		cli_options.list_encoders = vm.count("listencoders") > 0;
		cli_options.list_outputs = vm.count("listoutputs") > 0;

		if (cli_options.show_help) {
			std::cout << desc << "\n";
			return Ret::success;
		}

		if (should_print_lists())
			// will be properly handled later.
			return Ret::success;

		// only called here because it checks required fields, and when are trying to
		// use `help` or `list*` we want to ignore required fields.
		po::notify(vm);
	}
	catch (po::error& e) {
		std::cerr << "ERROR: " << e.what() << std::endl << std::endl;
		std::cerr << desc << std::endl;
		return Ret::error_in_command_line;
	}

	return Ret::success;
}

void start_output_callback(void *data, calldata_t *params) {
	auto output = static_cast<obs_output_t*>(calldata_ptr(params, "output"));
	std::cout << ">>>> Output " << output << " start." << std::endl;
}

void stop_output_callback(void *data, calldata_t *params) {
	auto loop = static_cast<EventLoop*>(data);
	auto output = static_cast<obs_output_t*>(calldata_ptr(params, "output"));
	int code = calldata_int(params, "code");

	std::cout << ">>>> Output " << output << " stop. with code " << code << std::endl;
	loop->stop();
}

int main(int argc, char **argv) {
	try {
		int ret = parse_args(argc, argv);
		if (ret != Ret::success)
			return ret;

		if (cli_options.show_help) {
			// We've already printed help. Exit.
			return Ret::success;
		}

		// manages object context so that we don't have to call
		// obs_startup and obs_shutdown.
		OBSContext ObsScope("en-US", nullptr, nullptr);

		if (!obs_initialized()) {
			std::cerr << "Obs initialization failed." << std::endl;
			return Ret::error_obs;
		}

		// must be called before reset_video
		if (!detect_monitors()){
			std::cerr << "No monitors found!" << std::endl;
			return Ret::success;
		}

		// resets must be called before loading modules.
		reset_video(cli_options.monitor_to_record);
		reset_audio();
		obs_load_all_modules();

		// can only be called after loading modules and detecting monitors.
		if (do_print_lists())
			return Ret::success;

		OBSSource source = setup_video_input(cli_options.monitor_to_record);
		if (cli_options.audio_index >= 0){
			OBSSource audio_source = setup_audio_input(cli_options.audio_index, cli_options.audio_is_output);
		}

		// While the outputs are kept in scope, we will continue recording.
		Outputs output = setup_outputs(cli_options.encoder, cli_options.video_bitrate, cli_options.outputs_paths);

		EventLoop loop;

		// connect signal events.
		OBSSignal output_start, output_stop;
		for (auto o : output.outputs) {
			output_start.Connect(obs_output_get_signal_handler(o), "start", start_output_callback, &loop);
			output_stop.Connect(obs_output_get_signal_handler(o), "stop", stop_output_callback, &loop);
		}

		start_recording(output.outputs);

		loop.run();

		return Ret::success;
	}
	catch (std::exception& e) {
		std::cerr << "Unhandled Exception reached the top of main: "
			<< e.what() << ", application will now exit" << std::endl;
		return Ret::error_unhandled_exception;
	}
}

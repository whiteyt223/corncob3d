"""Build native and freestanding Wasm math core using available pinned tools."""
from pathlib import Path
import importlib.util,re
import shutil
import subprocess

ROOT=Path(__file__).resolve().parents[1]
(ROOT/'build').mkdir(exist_ok=True)
subprocess.run(['g++','-std=c++17','-O2','-Wall','-Wextra','-Werror','-shared','-fPIC',str(ROOT/'core/math.cpp'),'-o',str(ROOT/'build/libcorncob-math.so')],check=True)
zig=shutil.which('zig')
if not zig:
    spec=importlib.util.find_spec('ziglang')
    if not spec or not spec.origin:raise SystemExit('Install pinned tools/requirements.txt or provide zig on PATH')
    zig=str(Path(spec.origin).parent/'zig')
exports=['dsqrt','atn2','vmag','sin','cos','ssin','scos','atn','csqrt']
exports+=['angles_matrix','matrix_multiply','matrix_angles','matvmul','flight_state','flight_refresh','flight_throttle','flight_step','line_input','line_output','project_line','draw_line','draw_horizon','render_wire','render_point']
exports+=['world_input','world_output','world_local_matrix','prepare_object']
exports+=['raster_set_seed','raster_get_seed']
exports+=['pmvmul']
exports+=['prepare_wire']
exports+=['instrument_input','instrument_draw','instrument_erase','instrument_oil_color']
exports+=['video_memory','video_load','video_store','instrument_reset','instrument_draw_page','instrument_erase_page']
exports+=['table_add','clear_bullets','bullet_spawn','missile_spawn','projectile_move']
exports+=['flight_flaps']
exports+=['collision_fast','collision_box','collision_near']
exports+=['new_effect','effect_puff','effect_boom','effect_flotsam','effect_shards']
exports+=['combat_hit','combat_slam','complete_objective']
exports+=['runtime_state','clock_advance','legacy_ticks']
exports+=['table_remove','table_sort','universe_memory','decode_object','encode_object','distance_object','bring_tile','farm_tile','update_tiles','promote_objects','cache_far_objects']
exports+=['move_bullets','move_effects','weapon_configure','weapon_input']
exports+=['promote_cached','world_funeral','connect_lifecycle_world']
exports+=['contact_crash','contact_walk','contact_pairs','contact_home']
exports+=['lifecycle_reset','lifecycle_terminal','lifecycle_events','lifecycle_clear_events','aircraft_damage','aircraft_clear_damage','aircraft_resolve_crash','aircraft_eject','aircraft_pull_chute','aircraft_try_reenter','pilot_walk','lifecycle_key_event','lifecycle_key_down','lifecycle_readkey','lifecycle_scalekey','lifecycle_arrows']
exports+=['enemy_timed','enemy_hit','enemy_shell_hit','enemy_observer_vector','enemy_attract','enemy_initnumbers','enemy_change_walls','enemy_radar_event']
exports+=['score_decode_result','score_encode_result','score_decode_file','score_encode_file','score_encode_scored','score_compute','score_accumulate','score_progress','score_commit_sortie','score_workspace','score_workspace_size']
exports+=['cockpit_'+n for n in ['gunsight_page','clear_ticks_page','tick_page','controls_page','home_page','gunsight_visible','oil_update','temperature_update','engine_update','temperature_ax','prop_update','prop_visible']]
exports+=['viewport_words', 'viewport_active', 'viewport_select', 'viewport_set', 'views_configure', 'views_flags', 'views_input', 'views_result', 'view_point', 'view_disc', 'view_line', 'view_polygon', 'render_views_point', 'render_views_disc', 'render_views_wire', 'render_views_polygon', 'resolve_draw_color', 'epage_page', 'epage_state_page']
exports+=['aircraft_clear_damage', 'aircraft_damage', 'aircraft_damage_component', 'aircraft_eject', 'aircraft_pull_chute', 'aircraft_qcrashland', 'aircraft_resolve_crash', 'flight_step', 'lifecycle_arrows', 'pilot_move', 'pilot_walk']
exports+=['frame_geometry','aircraft_board','aircraft_board_pending']
exports+=['flow_shortcut_gate', 'flow_escape', 'flow_finalize_status', 'flow_home', 'flow_commit_gate', 'flow_career_flags', 'flow_theater_totals', 'flow_start_marker', 'flow_cfg_active', 'flow_encode_theater']
exports+=['camera_prepare','view_input','view_remote_input','camera_towards','camera_after_world','camera_palette']
exports+=['enemy_palette_event']
exports+=['flush_tile','flush_tiles','donewmiss','doxp','transition_events']
exports+=['aircraft_damage_bar','aircraft_fullsize']
exports+=['career_file_size', 'career_encode', 'career_decode', 'career_active', 'career_name_gate', 'career_new', 'career_select_gate', 'career_activate', 'career_stockade', 'career_resurrect', 'career_resurrect_theater', 'career_after_sortie', 'career_new_theater', 'career_link_theater', 'career_select_theater', 'browser_workspace']
exports+=['hud_button', 'hud_damage', 'hud_flash_damage', 'hud_damage_indicator_init', 'hud_damage_init', 'hud_redraw_buttons', 'hud_flash_alt', 'hud_flash_eject', 'hud_flash_stall', 'hud_altitude_warning', 'hud_complete_button', 'radio_draw', 'radio_begin_queued', 'radio_open', 'radio_update', 'radio_reset', 'radio_state']
exports+=['enemy_rescue_request','enemy_distance_object']
exports+=['sortie_reset', 'sortie_stage', 'sortie_requests', 'sortie_beep', 'sortie_frame_end', 'sortie_begin_exit', 'sortie_after_near', 'sortie_config_hash', 'sortie_config_unhash', 'sortie_after_config', 'sortie_after_results', 'sortie_after_world', 'sortie_timer_reset', 'sortie_timer_read', 'sortie_add_elapsed', 'sortie_dvel_average', 'sortie_palette', 'color_input_reset', 'color_input', 'color_input_palette_dirty', 'world_file_buffer', 'world_file_capacity', 'world_read_file', 'world_write_file', 'world_io_result', 'world_allocate', 'world_clean_objects', 'world_update_tiles', 'audio_reset', 'audio_clock_advance', 'audio_write', 'audio_register_init', 'audio_init_voices', 'audio_voice_on', 'audio_voice_off', 'audio_preset', 'audio_snare', 'audio_shot', 'audio_missile', 'audio_crash', 'audio_crash_landing', 'audio_collision_blam', 'audio_explosion', 'audio_big_boom', 'audio_stall', 'audio_engine', 'audio_sound_off', 'audio_sound_on', 'audio_pause', 'audio_eject', 'audio_landing_screech', 'audio_timer_tick', 'audio_ambient_frame', 'audio_ambient_active', 'audio_register_ptr', 'audio_event_ptr', 'audio_event_capacity', 'audio_event_sequence', 'audio_raw_ticks', 'audio_event_at', 'audio_event_tick_at']
exports+=['initperms', 'generated_world_result', 'startup_seed', 'startup_random_seed', 'startup_common_flags', 'startup_pilot_options', 'startup_load_cfg', 'startup_before_world', 'startup_after_world', 'startup_generated_observer', 'startup_theater_params', 'startup_sky_detail', 'startup_palette', 'startup_ground_buffer', 'startup_ground', 'startup_matrices', 'sortie_stage', 'high_altitude_stage', 'high_altitude_begin', 'high_altitude_after_flush', 'high_altitude_after_near', 'high_altitude_after_presentation', 'high_altitude_after_stars']
exports+=['audio_reset', 'audio_clock_advance', 'audio_write', 'audio_register_init', 'audio_init_voices', 'audio_voice_on', 'audio_voice_off', 'audio_preset', 'audio_snare', 'audio_shot', 'audio_missile', 'audio_crash', 'audio_crash_landing', 'audio_stop_blam', 'audio_scrape_stop', 'audio_reset_missile_patch', 'audio_tower', 'audio_collision_blam', 'audio_explosion', 'audio_big_boom', 'audio_stall', 'audio_engine', 'audio_sound_off', 'audio_sound_on', 'audio_pause', 'audio_eject', 'audio_landing_screech', 'audio_timer_tick', 'audio_ambient_frame', 'audio_ambient_active', 'audio_register_ptr', 'audio_event_ptr', 'audio_event_capacity', 'audio_event_sequence', 'audio_raw_ticks', 'audio_event_at', 'audio_event_tick_at']
for header in ['startup','high_altitude','late_weapons','camera_visual','stars','career_management','career_scores','pause','map_controls','pilot_menu','airfield_summary','world_view','ground_stars','result_pictures','modal_controls','title_animation','modal_command','menu_controls','joystick','towers_info','edition','demo','demo_frame','modal_child','world_io','builder']:
    declarations=re.sub(r'/\*.*?\*/|//[^\n]*','',(ROOT/'core'/f'{header}.hpp').read_text(),flags=re.S)
    exports+=re.findall(r'\bcc_(\w+)\([^;{}]*\);',declarations)
exports+=['views_custom_front','views_fill','sortie_take_beep','score_accumulate_mode','score_progress_mode','career_decode_mode']
exports+=['audio_editor_buffer','audio_editor_reset','audio_editor_commit']
exports=list(dict.fromkeys(exports))
subprocess.run([zig,'c++','-target','wasm32-freestanding','-std=c++17','-O2','-g0','-nostdlib','-Wl,--strip-all','-Wl,--no-entry',
                *['-Wl,--export=cc_'+n for n in exports],
                '-Wl,--export=cc_movement_input','-Wl,--export=cc_movement_output','-Wl,--export=cc_move','-Wl,--export=cc_draw_disc','-Wl,--export=cc_image_input','-Wl,--export=cc_image_output','-Wl,--export=cc_decode_image_buffer',
                '-Wl,--export=cc_vector_input','-Wl,--export=cc_vector_output','-Wl,--export=cc_vector_run',
                '-Wl,--export=cc_projection_input','-Wl,--export=cc_projection_output','-Wl,--export=cc_project_point',
                *['-Wl,--export='+n for n in ['cc_raster_vertices','cc_framebuffer','cc_raster_spans','cc_raster_span_count','cc_clear','cc_draw_polygon']],
                *['-Wl,--export='+n for n in ['cc_clip_input','cc_clip_output','cc_clip_count','cc_clip_polygon','cc_scene_matrix','cc_scene_origin','cc_scene_vertices','cc_scene_angles','cc_render_polygon','cc_render_disc','cc_scene_octant']],
                *[str(ROOT/'core'/name) for name in ['math.cpp','flight_math.cpp','orientation.cpp','flight.cpp','movement.cpp','image.cpp','vector.cpp','projection.cpp','raster.cpp','disc.cpp','clip.cpp','scene.cpp','wire.cpp','world.cpp','instruments.cpp','projectiles.cpp','collision.cpp','effects.cpp','combat.cpp','runtime.cpp','universe.cpp','cockpit_overlays.cpp','cockpit_state.cpp','weapons.cpp','scoring.cpp','enemy_callbacks.cpp','contacts.cpp','flight_lifecycle.cpp','persistence.cpp','lifecycle_world.cpp','viewport.cpp','frame_geometry.cpp','mission_flow.cpp','camera.cpp','world_transition.cpp','career.cpp','browser_workspace.cpp','hud.cpp','sortie.cpp','color_input.cpp','world_io.cpp','audio.cpp','generated_world.cpp','startup.cpp','high_altitude.cpp','startup_matrices.cpp','late_weapons.cpp','camera_visual.cpp','stars.cpp','career_management.cpp','career_scores.cpp','pause.cpp','map_controls.cpp','pilot_menu.cpp','airfield_summary.cpp','world_view.cpp','ground_stars.cpp','result_pictures.cpp','modal_controls.cpp','title_animation.cpp','modal_command.cpp','menu_controls.cpp','joystick.cpp','towers_info.cpp','edition.cpp','demo.cpp','demo_frame.cpp','modal_child.cpp','builder.cpp']],'-o',str(ROOT/'build/corncob-math.wasm')],check=True)
print('Built native shared library and WebAssembly math core')

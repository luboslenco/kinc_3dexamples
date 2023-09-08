#include <kinc/graphics4/graphics.h>
#include <kinc/graphics4/indexbuffer.h>
#include <kinc/graphics4/pipeline.h>
#include <kinc/graphics4/shader.h>
#include <kinc/graphics4/vertexbuffer.h>
#include <kinc/graphics4/rendertarget.h>
#include <kinc/io/filereader.h>
#include <kinc/system.h>

#include <assert.h>
#include <stdlib.h>

static kinc_g4_shader_t vertex_shader;
static kinc_g4_shader_t fragment_shader;
static kinc_g4_pipeline_t pipeline;
static kinc_g4_vertex_buffer_t vertices;
static kinc_g4_index_buffer_t indices;
static kinc_g4_shader_t vertex_shader_fs;
static kinc_g4_shader_t fragment_shader_fs;
static kinc_g4_pipeline_t pipeline_fs;
static kinc_g4_vertex_buffer_t vertices_fs;
static kinc_g4_index_buffer_t indices_fs;
static kinc_g4_texture_unit_t tex_unit;
static kinc_g4_render_target_t target;

#define HEAP_SIZE 1024 * 1024
static uint8_t *heap = NULL;
static size_t heap_top = 0;

static void *allocate(size_t size) {
	size_t old_top = heap_top;
	heap_top += size;
	assert(heap_top <= HEAP_SIZE);
	return &heap[old_top];
}

static void update(void *data) {
	kinc_g4_begin(0);

	for (int i = 0; i < 6; ++i) {
		kinc_g4_set_render_target_face(&target, i);
		kinc_g4_clear(KINC_G4_CLEAR_COLOR, 0, 0.0f, 0);
		kinc_g4_set_pipeline(&pipeline);
		kinc_g4_set_vertex_buffer(&vertices);
		kinc_g4_set_index_buffer(&indices);
		kinc_g4_draw_indexed_vertices();
	}

	kinc_g4_restore_render_target();
	kinc_g4_clear(KINC_G4_CLEAR_COLOR, 0, 0.0f, 0);
	kinc_g4_set_pipeline(&pipeline_fs);
	kinc_g4_render_target_use_color_as_texture(&target, tex_unit);
	kinc_g4_set_vertex_buffer(&vertices_fs);
	kinc_g4_set_index_buffer(&indices_fs);
	kinc_g4_draw_indexed_vertices();

	kinc_g4_end(0);
	kinc_g4_swap_buffers();
}

static void load_shader(const char *filename, kinc_g4_shader_t *shader, kinc_g4_shader_type_t shader_type) {
	kinc_file_reader_t file;
	kinc_file_reader_open(&file, filename, KINC_FILE_TYPE_ASSET);
	size_t data_size = kinc_file_reader_size(&file);
	uint8_t *data = allocate(data_size);
	kinc_file_reader_read(&file, data, data_size);
	kinc_file_reader_close(&file);
	kinc_g4_shader_init(shader, data, data_size, shader_type);
}

int kickstart(int argc, char **argv) {
	kinc_init("Example", 1024, 768, NULL, NULL);
	kinc_set_update_callback(update, NULL);

	heap = (uint8_t *)malloc(HEAP_SIZE);
	assert(heap != NULL);

	load_shader("shader.vert", &vertex_shader, KINC_G4_SHADER_TYPE_VERTEX);
	load_shader("shader.frag", &fragment_shader, KINC_G4_SHADER_TYPE_FRAGMENT);

	kinc_g4_vertex_structure_t structure;
	kinc_g4_vertex_structure_init(&structure);
	kinc_g4_vertex_structure_add(&structure, "pos", KINC_G4_VERTEX_DATA_F32_3X);
	kinc_g4_pipeline_init(&pipeline);
	pipeline.vertex_shader = &vertex_shader;
	pipeline.fragment_shader = &fragment_shader;
	pipeline.input_layout[0] = &structure;
	pipeline.input_layout[1] = NULL;
	pipeline.depth_write = true;
	pipeline.depth_mode = KINC_G4_COMPARE_LESS;
	kinc_g4_pipeline_compile(&pipeline);

	kinc_g4_render_target_init_cube(&target, 512, KINC_G4_RENDER_TARGET_FORMAT_32BIT, 0, 0);

	kinc_g4_vertex_buffer_init(&vertices, 3, &structure, KINC_G4_USAGE_STATIC, 0);
	{
		float *v = kinc_g4_vertex_buffer_lock_all(&vertices);
		int i = 0;

		v[i++] = -1.0;
		v[i++] = kinc_g4_render_targets_inverted_y() ? -1.0 : 1.0;
		v[i++] = 0.0;

		v[i++] = 1.0;
		v[i++] = kinc_g4_render_targets_inverted_y() ? -1.0 : 1.0;
		v[i++] = 0.0;

		v[i++] = 0.0;
		v[i++] = kinc_g4_render_targets_inverted_y() ? 1.0 : -1.0;
		v[i++] = 0.0;

		kinc_g4_vertex_buffer_unlock_all(&vertices);
	}

	kinc_g4_index_buffer_init(&indices, 3, KINC_G4_INDEX_BUFFER_FORMAT_16BIT, KINC_G4_USAGE_STATIC);
	{
		uint16_t *i = (uint16_t *)kinc_g4_index_buffer_lock_all(&indices);
		i[0] = 0;
		i[1] = 1;
		i[2] = 2;
		kinc_g4_index_buffer_unlock_all(&indices);
	}

	load_shader("shader_fs.vert", &vertex_shader_fs, KINC_G4_SHADER_TYPE_VERTEX);
	load_shader("shader_fs.frag", &fragment_shader_fs, KINC_G4_SHADER_TYPE_FRAGMENT);

	kinc_g4_vertex_structure_t structure_fs;
	kinc_g4_vertex_structure_init(&structure_fs);
	kinc_g4_vertex_structure_add(&structure_fs, "pos", KINC_G4_VERTEX_DATA_F32_2X);
	kinc_g4_pipeline_init(&pipeline_fs);
	pipeline_fs.vertex_shader = &vertex_shader_fs;
	pipeline_fs.fragment_shader = &fragment_shader_fs;
	pipeline_fs.input_layout[0] = &structure_fs;
	pipeline_fs.input_layout[1] = NULL;
	kinc_g4_pipeline_compile(&pipeline_fs);
	tex_unit = kinc_g4_pipeline_get_texture_unit(&pipeline_fs, "tex");

	kinc_g4_vertex_buffer_init(&vertices_fs, 3, &structure_fs, KINC_G4_USAGE_STATIC, 0);
	{
		float *v = kinc_g4_vertex_buffer_lock_all(&vertices_fs);
		int i = 0;

		v[i++] = -1.0;
		v[i++] = -1.0;

		v[i++] = 3.0;
		v[i++] = -1.0;

		v[i++] = -1.0;
		v[i++] = 3.0;

		kinc_g4_vertex_buffer_unlock_all(&vertices_fs);
	}

	kinc_g4_index_buffer_init(&indices_fs, 3, KINC_G4_INDEX_BUFFER_FORMAT_16BIT, KINC_G4_USAGE_STATIC);
	{
		uint16_t *i = (uint16_t *)kinc_g4_index_buffer_lock_all(&indices_fs);
		i[0] = 0;
		i[1] = 1;
		i[2] = 2;
		kinc_g4_index_buffer_unlock_all(&indices_fs);
	}

	kinc_start();

	return 0;
}

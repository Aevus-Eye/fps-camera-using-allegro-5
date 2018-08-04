#include<allegro5\allegro.h>
#include<allegro5\allegro_font.h>
#include<allegro5\allegro_ttf.h>
#include<allegro5\allegro_color.h>
#include<allegro5\allegro_primitives.h>
#include<allegro5\allegro_image.h>
#include<vector>

#define FOV	(PI/2) //90 degrees Field Of View(vertical not horizontal)

#define SCREEN_WIDTH	(100 * 16)
#define SCREEN_HEIGHT	(100 * 9)

#define MOVE_SPEED	.03
#define MOUSE_SPEED	500. //higher = lower speed

#define LOOP(var,max)	for(var=0;var<max;++var)

class mouse_t {
public:
	int x = 0, y = 0;//xy location of mouse
	float cax, cay, caz;//camera angle
	float ax, ay;//xy angle of mouse
	void move_mouse(int dx, int dy) {
		x += dx;
		y += dy;
		ax = x / MOUSE_SPEED;//angle x: 1 rotation = 2*PI
		ay = y / -MOUSE_SPEED;//angle y: goes from -PI/2 to PI/2
		al_set_mouse_xy(al_get_current_display(), SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2);
		if (ay > PI / 2 - 0.00001) {
			ay = PI / 2 - 0.00001;
			y = ((PI / 2 - 0.00001)*-MOUSE_SPEED);
		}
		if (ay < -PI / 2 + 0.00001) {
			ay = -PI / 2 + 0.00001;
			y = ((-PI / 2 + 0.00001)*-MOUSE_SPEED);
		}
		//--------------camera calculations----------------
		cax = cos(ay)*cos(ax);
		caz = cos(ay)*sin(ax);
		cay = sin(ay);
	}
};

class player_t {
	bool keys[ALLEGRO_KEY_MAX] = { 0 };
public:
	mouse_t m;
	float x = 0, y = 0, z = 3;
	void press_key(int key) {
		keys[key] = 1;
	}
	void unpress_key(int key) {
		keys[key] = 0;
	}
	bool get_key(int key) {
		return keys[key];
	}
	void move_player() {
		if (keys[ALLEGRO_KEY_W]) {
			x += MOVE_SPEED * cos(m.ax);
			z += MOVE_SPEED * sin(m.ax);
		}
		if (keys[ALLEGRO_KEY_S]) {
			x -= MOVE_SPEED * cos(m.ax);
			z -= MOVE_SPEED * sin(m.ax);
		}
		if (keys[ALLEGRO_KEY_D]) {
			z += MOVE_SPEED * cos(-m.ax);
			x += MOVE_SPEED * sin(-m.ax);
		}
		if (keys[ALLEGRO_KEY_A]) {
			z -= MOVE_SPEED * cos(-m.ax);
			x -= MOVE_SPEED * sin(-m.ax);
		}
		if (keys[ALLEGRO_KEY_SPACE])
			y += MOVE_SPEED;
		if (keys[ALLEGRO_KEY_CAPSLOCK] || keys[ALLEGRO_KEY_LSHIFT])
			y -= MOVE_SPEED;
		if (keys[ALLEGRO_KEY_ESCAPE])
			exit(0);
	}
};

class meshv2_t {
	std::vector<ALLEGRO_BITMAP*> textures;
	std::vector<std::vector<ALLEGRO_VERTEX>> mesh;
public:
	void add_to_mesh(ALLEGRO_VERTEX*add, size_t amount_of_vertices, ALLEGRO_TRANSFORM*transform, ALLEGRO_BITMAP* texture) {
		int i;
		LOOP(i, textures.size()) {
			if (textures[i] == texture) {
				int old_mesh_size = mesh[i].size();
				mesh[i].resize(old_mesh_size + amount_of_vertices);
				memcpy(&(mesh[i][old_mesh_size]), add, amount_of_vertices * sizeof(ALLEGRO_VERTEX));
				LOOP(i, amount_of_vertices) {
					al_transform_coordinates_3d(transform, &mesh.back()[i + old_mesh_size].x, &mesh.back()[i + old_mesh_size].y, &mesh.back()[i + old_mesh_size].z);
				}
				return;
			}
		}
		textures.push_back(texture);
		mesh.emplace_back(amount_of_vertices);
		memcpy(&(mesh.back()[0]), add, amount_of_vertices * sizeof(ALLEGRO_VERTEX));
		LOOP(i, amount_of_vertices) {
			al_transform_coordinates_3d(transform, &mesh.back()[i].x, &mesh.back()[i].y, &mesh.back()[i].z);
		}
	}
	void reset_mesh() {
		int i;
		LOOP(i, textures.size()) {
			mesh[i].resize(0);
		}
		mesh.resize(0);
		textures.resize(0);
	}
	void draw() {
		int i;
		LOOP(i, textures.size()) {
			al_draw_prim(mesh[i].data(), NULL, textures[i], 0, mesh[i].size(), ALLEGRO_PRIM_TRIANGLE_LIST);//drawing the mesh as before
		}
	}
};


void init() {
	al_init();
	al_init_image_addon();
	al_init_primitives_addon();
	al_init_ttf_addon();
	al_init_font_addon();
	al_install_keyboard();
	al_install_mouse();
	al_set_new_display_option(ALLEGRO_DEPTH_SIZE, 16, ALLEGRO_SUGGEST);
}

void setup_3d_projection() {
	ALLEGRO_TRANSFORM projection;
	al_identity_transform(&projection);
	al_translate_transform_3d(&projection, 0, 0, 0);
	double fov = tan(FOV / 2);
	al_perspective_transform(&projection, -1 * SCREEN_WIDTH / SCREEN_HEIGHT * fov, fov, 1, fov*SCREEN_WIDTH / SCREEN_HEIGHT, -fov, 1000);
	al_use_projection_transform(&projection);
}

int main() {
	init();
	int i, j;
	auto display = al_create_display(SCREEN_WIDTH, SCREEN_HEIGHT);
	auto q = al_create_event_queue();
	auto fps = al_create_timer(1 / 60.);
	meshv2_t mesh;
	ALLEGRO_EVENT ev;
	ALLEGRO_TRANSFORM tra, cam, curprotra;
	player_t p;
	auto font = al_load_ttf_font("fonts/DeansgateCondensed-Bold.ttf", 16, 0);//just a generic font for testing
	al_register_event_source(q, al_get_timer_event_source(fps));
	al_register_event_source(q, al_get_mouse_event_source());
	al_register_event_source(q, al_get_display_event_source(display));
	al_register_event_source(q, al_get_keyboard_event_source());
	al_start_timer(fps);
	auto image = al_load_bitmap("images/test_image.jpeg");
	al_set_mouse_xy(display, SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2);
	ALLEGRO_VERTEX cube[] = {
	{-1.0f,-1.0f,-1.0f,  0,0,al_map_rgb_f(1,1,1)},
	{-1.0f,-1.0f, 1.0f,	 0,0,al_map_rgb_f(1,1,1)},
	{-1.0f, 1.0f, 1.0f,	 0,0,al_map_rgb_f(1,1,1)},

	{1.0f, 1.0f,-1.0f, 	 0,0,al_map_rgb_f(0,1,1)},
	{-1.0f,-1.0f,-1.0f,	 0,0,al_map_rgb_f(0,1,1)},
	{-1.0f, 1.0f,-1.0f,	 0,0,al_map_rgb_f(0,1,1)},

	{1.0f,-1.0f, 1.0f,	 0,0,al_map_rgb_f(1,0,1)},
	{-1.0f,-1.0f,-1.0f,	 0,0,al_map_rgb_f(1,0,1)},
	{1.0f,-1.0f,-1.0f,	 0,0,al_map_rgb_f(1,0,1)},

	{1.0f, 1.0f,-1.0f,	 0,0,al_map_rgb_f(1,1,0)},
	{1.0f,-1.0f,-1.0f,	 0,0,al_map_rgb_f(1,1,0)},
	{-1.0f,-1.0f,-1.0f,	 0,0,al_map_rgb_f(1,1,0)},

	{-1.0f,-1.0f,-1.0f,	 0,0,al_map_rgb_f(0,0,1)},
	{-1.0f, 1.0f, 1.0f,	 0,0,al_map_rgb_f(0,0,1)},
	{-1.0f, 1.0f,-1.0f,	 0,0,al_map_rgb_f(0,0,1)},

	{1.0f,-1.0f, 1.0f,	 0,0,al_map_rgb_f(0,1,0)},
	{-1.0f,-1.0f, 1.0f,	 0,0,al_map_rgb_f(0,1,0)},
	{-1.0f,-1.0f,-1.0f,	 0,0,al_map_rgb_f(0,1,0)},

	{-1.0f, 1.0f, 1.0f,	 0,0,al_map_rgb_f(1,0,0)},
	{-1.0f,-1.0f, 1.0f,	 0,0,al_map_rgb_f(1,0,0)},
	{1.0f,-1.0f, 1.0f,	 0,0,al_map_rgb_f(1,0,0)},

	{1.0f, 1.0f, 1.0f,	 0,0,al_map_rgb_f(0,0,0)},
	{1.0f,-1.0f,-1.0f,	 0,0,al_map_rgb_f(0,0,0)},
	{1.0f, 1.0f,-1.0f,	 0,0,al_map_rgb_f(0,0,0)},

	{1.0f,-1.0f,-1.0f,	 0,0,al_map_rgb_f(1,1,1)},
	{1.0f, 1.0f, 1.0f,	 0,0,al_map_rgb_f(1,1,1)},
	{1.0f,-1.0f, 1.0f,	 0,0,al_map_rgb_f(1,1,1)},

	{1.0f, 1.0f, 1.0f,	 0,0,al_map_rgb_f(0,1,1)},
	{1.0f, 1.0f,-1.0f,	 0,0,al_map_rgb_f(0,1,1)},
	{-1.0f, 1.0f,-1.0f,	 0,0,al_map_rgb_f(0,1,1)},

	{1.0f, 1.0f, 1.0f,	 0,0,al_map_rgb_f(1,0,1)},
	{-1.0f, 1.0f,-1.0f,	 0,0,al_map_rgb_f(1,0,1)},
	{-1.0f, 1.0f, 1.0f,	 0,0,al_map_rgb_f(1,0,1)},

	{1.0f, 1.0f, 1.0f,	 0,0,al_map_rgb_f(1,1,0)},
	{-1.0f, 1.0f, 1.0f,	 0,0,al_map_rgb_f(1,1,0)},
	{1.0f,-1.0f, 1.0f,   0,0,al_map_rgb_f(1,1,0)},
	};
	ALLEGRO_VERTEX ground[] = {
	{ 1.0f,0,1.0f,al_get_bitmap_width(image),al_get_bitmap_height(image),al_map_rgb_f(0,1,1) },
	{ -1.0f,0,1.0f,0,al_get_bitmap_height(image),al_map_rgb_f(1,0,1) },
	{ -1.0f,0,-1.0f,0,0,al_map_rgb_f(1,1,0) },

	{ 1.0f,0,1.0f,al_get_bitmap_width(image),al_get_bitmap_height(image),al_map_rgb_f(0,0,1) },
	{ 1.0f,0,-1.0f,al_get_bitmap_width(image),0,al_map_rgb_f(1,0,0) },
	{ -1.0f,0,-1.0f,0,0,al_map_rgb_f(0,1,0) },
	};

	//initialize the mesh
	al_identity_transform(&tra);
	mesh.add_to_mesh(cube, std::size(cube), &tra,NULL);
	al_translate_transform_3d(&tra, 3, 0, 0);
	mesh.add_to_mesh(cube, std::size(cube), &tra,NULL);
	al_identity_transform(&tra);
	al_scale_transform_3d(&tra, 10, 1, 10);
	al_translate_transform_3d(&tra, 0, 6, 0);
	mesh.add_to_mesh(ground, std::size(ground), &tra,image);

	while (1) {
		al_wait_for_event(q, &ev);
		switch (ev.type) {
		case ALLEGRO_EVENT_KEY_DOWN:
			p.press_key(ev.keyboard.keycode);
			break;
		case ALLEGRO_EVENT_KEY_UP:
			p.unpress_key(ev.keyboard.keycode);
			break;
		case ALLEGRO_EVENT_MOUSE_AXES:
			p.m.move_mouse(ev.mouse.dx, ev.mouse.dy);
			break;
		case ALLEGRO_EVENT_TIMER:
			p.move_player();

			curprotra = *al_get_current_projection_transform();
			setup_3d_projection();

			al_clear_to_color(al_map_rgb_f(0.5, 0.5, 0.5));
			al_set_render_state(ALLEGRO_DEPTH_TEST, 1);
			al_clear_depth_buffer(1);

			//	----------------camera transform---------------
			al_build_camera_transform(&cam, p.x, p.y, p.z,
				p.x + p.m.cax, p.y + p.m.cay, p.z + p.m.caz,
				0, 1, 0);
			al_use_transform(&cam);

			//draw 3d things
			mesh.draw();
			//draw things that arent in the mesh. you can also draw 2d primitives
			al_identity_transform(&tra);
			al_translate_transform_3d(&tra, 0, -3, 0);
			al_compose_transform(&tra, &cam);
			al_use_transform(&tra);
			//al_draw_prim(cube, NULL, NULL, 0, std::size(cube), ALLEGRO_PRIM_TRIANGLE_LIST);
			al_draw_filled_circle(0, 0, 1, al_map_rgba_f(0,1,0.5,0.5));//this circle will always be pixel perfect, no matter how far you zoom in

			//restore projection
			al_identity_transform(&tra);
			al_use_transform(&tra);
			al_use_projection_transform(&curprotra);
			al_set_render_state(ALLEGRO_DEPTH_TEST, 0);

			//draw 2d things
			al_draw_multiline_textf(font, al_map_rgb_f(0.8, 0.1, 0.2), 50, 50, 0, 0, 0, "normalized(2PI=1) ax_%.4f ay_%.4f not_normalized cax_%.4f cay_%.4f caz_%.4f", p.m.ax / PI / 2, p.m.ay / PI / 2, p.m.cax, p.m.cay, p.m.caz);
			al_flip_display();
			break;
		}
	}
}


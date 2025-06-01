#include "Render.h"
#include <Windows.h>
#include <GL\GL.h>
#include <GL\GLU.h>
#include <iostream>
#include <iomanip>
#include <sstream>
#include "GUItextRectangle.h"
#include "MyShaders.h"
#include "Texture.h"
#include <mmsystem.h>

#include "ObjLoader.h"


#include "debout.h"
#pragma comment(lib, "winmm.lib") 


void PlayMusic(std::wstring music_name)
{
	PlaySound((LPCTSTR)music_name.c_str(), NULL, SND_FILENAME | SND_ASYNC | SND_LOOP);
}

void PlayWinSound(std::wstring music_name)
{
	PlaySound((LPCTSTR)music_name.c_str(), NULL, SND_FILENAME | SND_ASYNC);
}


//внутренняя логика "движка"
#include "MyOGL.h"
extern OpenGL gl;
#include "Light.h"
Light light;
#include "Camera.h"
Camera camera;
double _global_delta = 0;

bool texturing = true;
bool lightning = true;
bool alpha = true;

int first_done = 0;
int second_done = 0;
int third_done = 0;
int fourth_done = 0;
int rubbish1_done = 0;
int rubbish2_done = 0;
int win = 0;
//переключение режимов освещения, текстурирования, альфаналожения
void switchModes(OpenGL *sender, KeyEventArg arg)
{
	//конвертируем код клавиши в букву
	auto key = LOWORD(MapVirtualKeyA(arg.key, MAPVK_VK_TO_CHAR));

	switch (key)
	{
	case 'L':
		lightning = !lightning;
		break;
	case 'T':
		texturing = !texturing;
		break;
	case 'A':
		alpha = !alpha;
		break;
	}
}
double t1 = 12;
double t2 = 34;
double t3 = 400;
double t4 = 69;
double t5 = 180;
double ang1 = 45;
double ang2 = 90;
double ang3 = 234;
double ang4 = 67;
double ang6 = 140;

double fl = 0;

int count = 0;

void switchModesCustom(OpenGL* sender, KeyEventArg arg)
{
	//конвертируем код клавиши в букву
	auto key = LOWORD(MapVirtualKeyA(arg.key, MAPVK_VK_TO_CHAR));

	switch (key)
	{
	case '6':
		fl = 6;
		break;
	case '1':
		if (first_done == 0) {
			fl = 1;
		}
		else {
			fl = 0;
		}

		break;
	case '2':
		if (second_done == 0) {
			fl = 2;
		}
		else
		{
			fl = 0;
		}
		break;
	case '3':
		if (third_done == 0) {
			fl = 3;
		}
		else
		{
			fl = 0;
		}
		break;
	case '4':
		if (fourth_done == 0) {
			fl = 4;
		}
		else
		{
			fl = 0;
		}
		break;
	case '5':
		fl = 5;
		break;

	case 'Q':
		if (fl == 1 and first_done == 0)
			t1 = t1 + _global_delta/20;
		if (fl == 2 and second_done == 0)
			t2 = t2 + _global_delta / 20;
		if (fl == 3 and third_done == 0)
			t3 = t3 + _global_delta / 20;
		if (fl == 4 and fourth_done == 0)
			t4 = t4 + _global_delta / 20;
		if (fl == 5)
			t5 = t5 + _global_delta / 20;
		break;
	case 'E':
		if (fl == 1 and first_done == 0)
			t1 = t1 - _global_delta / 20;
		if (fl == 2 and second_done == 0)
			t2 = t2 - _global_delta / 20;
		if (fl == 3 and third_done == 0)
			t3 = t3 - _global_delta / 20;
		if (fl == 4 and fourth_done == 0)
			t4 = t4 - _global_delta / 20;
		if (fl == 5)
			t5 = t5 - _global_delta / 20;
		break;
	case 'W':
		if (fl == 1 and first_done == 0)
			ang1 = ang1 + _global_delta / 20;
		if (fl == 2 and second_done == 0)
			ang2 = ang2 + _global_delta / 20;
		if (fl == 3 and third_done == 0)
			ang3 = ang3 + _global_delta / 20;
		if (fl == 4 and fourth_done == 0)
			ang4 = ang4 + _global_delta / 20;
		if (fl == 6)
			ang6 = ang6 + _global_delta / 20;
		break;
	case 'S':
		if (fl == 1 and first_done == 0)
			ang1 = ang1 - _global_delta / 20;
		if (fl == 2 and second_done == 0)
			ang2 = ang2 - _global_delta / 20;
		if (fl == 3 and third_done == 0)
			ang3 = ang3 - _global_delta / 20;
		if (fl == 4 and fourth_done == 0)
			ang4 = ang4 - _global_delta / 20;
		if (fl == 6)
			ang6 = ang6 - _global_delta / 20;
		break;
	case '0':
		t1 = t2 = t3 = t4 = ang1 = ang2 = ang3 = ang4 = 0;
	}
}

//умножение матриц c[M1][N1] = a[M1][N1] * b[M2][N2]
template<typename T, int M1, int N1, int M2, int N2>
void MatrixMultiply(const T* a, const T* b, T* c)
{
	for (int i = 0; i < M1; ++i)
	{
		for (int j = 0; j < N2; ++j)
		{
			c[i * N2 + j] = T(0);
			for (int k = 0; k < N1; ++k)
			{
				c[i * N2 + j] += a[i * N1 + k] * b[k * N2 + j];
			}
		}
	}
}

//Текстовый прямоугольничек в верхнем правом углу.
//OGL не предоставляет возможности для хранения текста
//внутри этого класса создается картинка с текстом (через виндовый GDI),
//в виде текстуры накладывается на прямоугольник и рисуется на экране.
//Это самый простой способ что то написать на экране
//но ооооочень не оптимальный
GuiTextRectangle text;

//айдишник для текстуры
GLuint texId;
//выполняется один раз перед первым рендером

ObjModel f;


Shader cassini_sh;
Shader phong_sh;
Shader vb_sh;
Shader simple_texture_sh;

Texture stankin_tex, vb_tex, monkey_tex, line_tex, cong_tex;
double timer = 0;


void initRender()
{

	cassini_sh.VshaderFileName = "shaders/v.vert";
	cassini_sh.FshaderFileName = "shaders/cassini.frag";
	cassini_sh.LoadShaderFromFile();
	cassini_sh.Compile();

	phong_sh.VshaderFileName = "shaders/v.vert";
	phong_sh.FshaderFileName = "shaders/light.frag";
	phong_sh.LoadShaderFromFile();
	phong_sh.Compile();

	vb_sh.VshaderFileName = "shaders/v.vert";
	vb_sh.FshaderFileName = "shaders/vb.frag";
	vb_sh.LoadShaderFromFile();
	vb_sh.Compile();

	simple_texture_sh.VshaderFileName = "shaders/v.vert";
	simple_texture_sh.FshaderFileName = "shaders/textureShader.frag";
	simple_texture_sh.LoadShaderFromFile();
	simple_texture_sh.Compile();

	stankin_tex.LoadTexture("textures/stankin.png");
	vb_tex.LoadTexture("textures/vb.png");
	monkey_tex.LoadTexture("textures/monkey.png");
	line_tex.LoadTexture("textures/stanok.png");
	cong_tex.LoadTexture("textures/congratulations.jpg");
	

	f.LoadModel("models//monkey.obj_m");
	//==============НАСТРОЙКА ТЕКСТУР================
	//4 байта на хранение пикселя
	glPixelStorei(GL_UNPACK_ALIGNMENT, 4);

	

	//================НАСТРОЙКА КАМЕРЫ======================
	camera.caclulateCameraPos();

	//привязываем камеру к событиям "движка"
	gl.WheelEvent.reaction(&camera, &Camera::Zoom);
	gl.MouseMovieEvent.reaction(&camera, &Camera::MouseMovie);
	gl.MouseLeaveEvent.reaction(&camera, &Camera::MouseLeave);
	gl.MouseLdownEvent.reaction(&camera, &Camera::MouseStartDrag);
	gl.MouseLupEvent.reaction(&camera, &Camera::MouseStopDrag);
	//==============НАСТРОЙКА СВЕТА===========================
	//привязываем свет к событиям "движка"
	gl.MouseMovieEvent.reaction(&light, &Light::MoveLight);
	gl.KeyDownEvent.reaction(&light, &Light::StartDrug);
	gl.KeyUpEvent.reaction(&light, &Light::StopDrug);
	//========================================================
	//====================Прочее==============================
	gl.KeyDownEvent.reaction(switchModes);
	text.setSize(512, 180);
	//========================================================
	   

	camera.setPosition(2, 1.5, 1.5);
	
	PlayMusic(L"sounds/music.wav");

}
struct Point {
	double x;
	double y;
	double z;
};
void normal(Point A, Point B, Point C) {
	Point BA{ A.x - B.x,A.y - B.y,A.z - B.z };
	Point BC{ C.x - B.x,C.y - B.y,C.z - B.z };
	Point N{ BA.y * BC.z - BA.z * BC.y,
	-BA.x * BC.z + BA.z * BC.x,
	BA.x * BC.y - BA.y * BC.x };
	//Нормализуем нормаль
	double l = sqrt(N.x * N.x + N.y * N.y + N.z * N.z);
	N.x /= l;
	N.y /= l;
	N.z /= l;
	glNormal3dv((double*)&N);


}
Point up(Point a, double h) {
	a.z = a.z + h;
	return a;
}
void colorLight(double d1, double d2, double d3, double a1, double a2, double a3, double tr, double tra) {
	float  amb[] = { a1, a2, a3, tr };
	float dif[] = { d1, d2, d3, tra };
	float spec[] = { 0, 0, 0, 1. };
	float sh = 0.2f * 256;
	glMaterialfv(GL_FRONT, GL_AMBIENT, amb);
	glMaterialfv(GL_FRONT, GL_DIFFUSE, dif);
	glMaterialfv(GL_FRONT, GL_SPECULAR, spec);
	glMaterialf(GL_FRONT, GL_SHININESS, sh);
	glShadeModel(GL_SMOOTH); 
}
void quad(Point a, Point b, Point c, Point d) {
	glBegin(GL_QUADS);
	glVertex3d(a.x, a.y, a.z);
	glVertex3d(b.x, b.y, b.z);
	glVertex3d(c.x, c.y, c.z);
	glVertex3d(d.x, d.y, d.z);
	glEnd();
}

void circle(double R1, double R2, double alfa, double beta, double h, double hfig = 1) {
	glPushMatrix();
	glScaled(pow(0.98, h), pow(0.98, h), pow(0.98, h));
	alfa = alfa * 3.14159265 / 180;
	beta = beta * 3.14159265 / 180;
	double step = (beta - alfa) / 100;
	Point A = { R1 * cos(alfa), R1 * sin(alfa), h };
	Point B = { R2 * cos(alfa), R2 * sin(alfa), h };
	quad(A, B, up(B, hfig), up(A, hfig));
	for (double t = alfa; t < beta; t = t + step) {
		Point D = { R1 * cos(t), R1 * sin(t), h };
		Point C = { R2 * cos(t), R2 * sin(t), h };
		quad(A, B, C, D);
		quad(up(A, hfig), up(B, hfig), up(C, hfig), up(D, hfig));
		quad(B, C, up(C,hfig), up(B, hfig));
		quad(D, A, up(A,hfig), up(D, hfig));
		A = D;
		B = C;
	}
	Point D = { R1 * cos(beta), R1 * sin(beta), h };
	Point C = { R2 * cos(beta), R2 * sin(beta), h };
	quad(C, D, up(D, hfig), up(C, hfig));
	glPopMatrix();
}
void section(double R1, double R2, double alfa, double beta, double h, double hfig = 1) {
	glPushMatrix();
	glScaled(pow(0.98, h), pow(0.98, h), pow(0.98, h));
	glBegin(GL_QUADS);
	alfa = alfa * 3.14159265 / 180;
	beta = beta * 3.14159265 / 180;
	Point A = { R1 * cos(alfa), R1 * sin(alfa), h };
	Point B = { R2 * cos(alfa), R2 * sin(alfa), h };
	Point D = { R1 * cos(beta), R1 * sin(beta), h };
	Point C = { R2 * cos(beta), R2 * sin(beta), h };
	quad(up(A, hfig), up(B, hfig), up(C, hfig), up(D, hfig));
	quad(B, C, up(C, hfig), up(B, hfig));
	quad(D, A, up(A, hfig), up(D, hfig));
	quad(C, D, up(D, hfig), up(C, hfig));
	quad(A, B, C, D);
	glEnd();
	glPopMatrix();
}

void second_quater() {
	if (fl == 2 and timer > 3) {
		colorLight(0., 0., 1, 1, 1, 1, 1, 1);
	}
	else
	colorLight(0, 0, 1, 0.2, 0.4, 0, 1, 1);
	section(2, 4, 270, 295, -3);
	section(2, 4, 295, 310, -2);
	section(2, 4, 310, 320, -1);
	section(2, 4, 320, 330, 0);
	if (fl == 2 and timer > 3) {
		colorLight(1, 0., 1, 1, 1, 1, 1, 1);
	}
	else
		colorLight(1, 0, 1, 1, 0.2, 1, 1, 1);
	circle(4, 5, 270, 330, 2);
	circle(5, 6, 270, 330, 0);
	circle(6, 7, 270, 330, 3);
	circle(7, 8, 270, 330, 1);
}
void first_quater() {

	if (fl == 1 and (timer > 3 or timer < 1)){
		colorLight(1, 0., 1, 1, 1, 1, 1, 1);
	}
	else
		colorLight(1, 0., 1, 0.2, 0.4, 0.2, 1, 1);
	glColor3d(0.7, 0, 0.7);
	circle(2, 6, 30, 60, 4, 0.5);
}
void third_quater() {
	if (fl == 3 and (timer > 3 or timer < 1)) {
		colorLight(0, 0., 1, 1, 1, 1, 1, 1);
	}
	else
	colorLight(0, 0, 1, 0.2, 0.4, 0, 1, 1);
	glColor3d(0, 0, 1);
	for (int i = 180; i < 270; i = i + 15) {
		section(2, 4, i, i + 15, (i-180)/15);
	}
	glColor3d(0.4, 1, 0.4);
	if (fl == 3 and timer > 3) {
		colorLight(0, 1., 0, 1, 1, 1, 1, 1);
	}
	else
	colorLight(0, 1, 0, 1, 0, 1, 1, 1);
		circle(4, 7, 240, 270, -2);
	if (fl == 3 and (timer > 3 or timer < 1)) {
		colorLight(1, 0., 0, 1, 1, 1, 1, 1);
	}
	else
	colorLight(1, 0, 0, 0, 0.2, 0.6, 1, 1);
	circle(4, 7, 180, 210, 2);
}
void fourtrh_quater() {
	if (fl == 4 and (timer > 3 or timer < 1)) {
		colorLight(0, 0., 1, 1, 1, 1, 1, 1);
	}
	else
		colorLight(0, 0, 1, 0.2, 0.4, 0, 1, 1);
	for (int i = 150; i < 180; i = i + 15) {
		section(2, 4, i, i + 15, -(i-150)/15);
	}
	glColor3d(1, 0, 0);
	if (fl == 4 and (timer > 3 or timer < 1)) {
		colorLight(1, 0., 0, 1, 1, 1, 1, 1);
	}
	else
		colorLight(1, 0, 0, 0, 0.2, 0.6, 1, 1);
	circle(4, 7, 150, 180, 2);
	glColor3d(0.2, 0.6, 1);
	if (fl == 4 and (timer > 3 or timer < 1)) {
		colorLight(0, 0., 0.8, 1, 1, 1, 1, 1);
	}
	else
		colorLight(0, 0, 0.8, 0.4, 0.6, 0, 1, 1);
	for (int i = 90; i < 150; i = i + 15) {
		section(2, 3, i, i + 15, 4-(i-90)/15);
	}
	circle(3, 4, 90, 150, 0);
	glColor3d(0.2, 0.6, 1);
	for (int i = 90; i < 150; i = i + 15) {
		section(4, 6, i, i + 15, 1+(i-90)/15);
	}

	circle(6, 8, 90, 150, 0);
}
void rubbish1() {
	if (fl == 5 and (timer > 3 or timer < 1)) {
		colorLight(1, 1, 1, 0, 0, 0, 1, 0.1);
	}
	else
	colorLight(1, 1, 1, 1, 1, 1, 1, 0.001);
	circle(1, 5, 342, 420, 2);
}
void rubbish2() {
	if (fl == 5 and (timer > 3 or timer < 1)) {
		colorLight(1, 1, 1, 0, 0, 0, 1, 0.1);
	}

	else
	colorLight(1, 1, 1, 1, 1, 1, 1, 0.001);
	for (int i = 400; i < 500; i = i + 15) {
		section(4, 6, i, i + 15, 2 + (i - 300) / 15);
	}
}
void rubbish3() {
	if (fl == 6 and (timer > 3 or timer < 1)) {
		colorLight(1, 1, 1, 0, 0, 0, 1, 0.1);
	}

	else
		colorLight(1, 1, 1, 1, 1, 1, 1, 0.001);
	for (int i =200; i < 540; i = i + 15) {
		section(4, 6, i+10, i + 45, 2 + (i-120) / 15);
	}
}
float view_matrix[16];
double full_time = 0;
int location = 0;
void Render(double delta_time)
{
	_global_delta = delta_time;
	timer += delta_time;
	full_time += delta_time;
	//натройка камеры и света
	//в этих функциях находятся OGLные функции
	//которые устанавливают параметры источника света
	//и моделвью матрицу, связанные с камерой.
	if (timer > 4)
		timer = 0;
	if (gl.isKeyPressed('F')) //если нажата F - свет из камеры
	{
		light.SetPosition(camera.x(), camera.y(), camera.z());
	}
	camera.SetUpCamera();
	//забираем моделвью матрицу сразу после установки камера
	//так как в ней отсуствуют трансформации glRotate...
	//она, фактически, является видовой.
	glGetFloatv(GL_MODELVIEW_MATRIX, view_matrix);



	light.SetUpLight();

	//рисуем оси
	gl.DrawAxes();



	glBindTexture(GL_TEXTURE_2D, 0);

	//включаем нормализацию нормалей
	//чтобв glScaled не влияли на них.

	glEnable(GL_NORMALIZE);
	glDisable(GL_LIGHTING);
	glDisable(GL_TEXTURE_2D);
	glDisable(GL_BLEND);

	//включаем режимы, в зависимости от нажания клавиш. см void switchModes(OpenGL *sender, KeyEventArg arg)
	if (lightning)
		glEnable(GL_LIGHTING);
	if (texturing)
	{
		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, 0); //сбрасываем текущую текстуру
	}

	if (alpha)
	{
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	}

	//=============НАСТРОЙКА МАТЕРИАЛА==============


	////настройка материала, все что рисуется ниже будет иметь этот метериал.
	////массивы с настройками материала
	//float  amb[] = { 0.2, 0.2, 0.2, 1. };
	//float dif[] = { 0.5, 0.5, 0.5, 0.5 };
	//float spec[] = { 0, 0, 0, 1. };
	//float sh = 0.2f * 256;
	////фоновая
	//glMaterialfv(GL_FRONT, GL_AMBIENT, amb);
	////дифузная
	//glMaterialfv(GL_FRONT, GL_DIFFUSE, dif);
	////зеркальная
	//glMaterialfv(GL_FRONT, GL_SPECULAR, spec);
	////размер блика
	//glMaterialf(GL_FRONT, GL_SHININESS, sh);

	////чтоб было красиво, без квадратиков (сглаживание освещения)
	//glShadeModel(GL_SMOOTH); //закраска по Гуро      
	////(GL_SMOOTH - плоская закраска)

//============ РИСОВАТЬ ТУТ ==============
	if (t1 > 360 or t1 < -360)
		t1 = 0;
	if (t2 > 360 or t2 < -360)
		t2 = 0;
	if (t3 > 360 or t3 < -360)
		t3 = 0;
	if (t4 > 360 or t4 < -360)
		t4 = 0;
	if (t5 > 360 or t5 < -360)
		t5 = 0;
	if (ang1 > 360 or ang1 < -360)
		ang1 = 0;
	if (ang2 > 360 or ang2 < -360)
		ang2 = 0;
	if (ang3 > 360 or ang3 < -360)
		ang3 = 0;
	if (ang4 > 360 or ang4 < -360)
		ang4 = 0;
	if (ang6 > 360 or ang6 < -360)
		ang6 = 0;
	if (t1 < 3 and t1 > -3) {
		if (ang1 < 3 and ang1 > -3) {
			first_done = 1;

		}
	}
	if (t2 < 3 and t2 > -3) {
		if (ang2 < 3 and ang2 > -3) {
			second_done = 1;
		}
	}
	if (t3 < 3 and t3 > -3) {
		if (ang3 < 3 and ang3 > -3) {
			third_done = 1;
		}
	}
	if (t4 < 3 and t4 > -3) {
		if (ang4 < 3 and ang4 > -3) {
			fourth_done = 1;
		}
	}
	if (t5 < 3 and t5 > -3) {
		rubbish1_done = 1;
	}
	else
	{
		rubbish1_done = 0;
	}
	if (ang6 < 3 and ang4 > -3) {
		rubbish2_done = 1;
	}
	else {
		rubbish2_done = 0;
	}
	if (first_done and second_done and third_done and fourth_done and rubbish1_done and rubbish2_done) {
		win = 1;
		if(win and count == 0)
		{
			PlayWinSound(L"sounds/win.wav");
			count++;
		}
		
	}
	

	glRotated(60, 1, 1, 1);
	glPushMatrix();
	glRotated(ang1, 1, 1, 0);
	glRotated(t1, 0, 1, 0);
	first_quater();
	glPopMatrix();
	glPushMatrix();
	glRotated(90, 1, 0, 1);
	glRotated(ang6, 0, 0, 1);
	rubbish3();
	glPopMatrix();
	glPopMatrix();
	glPushMatrix();
	glRotated(t5-60, 0, 0, 1);
	rubbish2();
	glPopMatrix();
	glPushMatrix();
	glRotated(ang2, 1, -1, 0);
	glRotated(t2, 0, 0, 1);
	second_quater();
	glPopMatrix();
	glPushMatrix();
	glRotated(t5-60, 0, 0, 1);
	rubbish1();
	glPopMatrix();
	glPushMatrix();
	glRotated(ang3, -1, -1, 0);
	glRotated(t3, 0, 0, 1);
	third_quater();
	glPopMatrix();
	glPushMatrix();
	glRotated(-90, 0, 1, 1);
	glRotated(ang6, 0, 0, 1);
	rubbish3();
	glPopMatrix();
	glPushMatrix();
	glRotated(ang4, -1, 1, 0);
	glRotated(t4, 0, 0, 1);
	fourtrh_quater();
	glPopMatrix();
	gl.KeyDownEvent.reaction(switchModesCustom);
	

	////рисуем квадратик с овалом Кассини!
	//
	//cassini_sh.UseShader();

	//location = glGetUniformLocationARB(cassini_sh.program, "Time");
	//glUniform1fARB(location, full_time);
	//location = glGetUniformLocationARB(cassini_sh.program, "size");
	//glUniform2fARB(location, 100, 100);
	//
	//glPushMatrix();

	//glTranslated(0, -1.2, 0);

	//
	//glBegin(GL_QUADS);
	//glNormal3d(0, 0, 1);
	//glTexCoord2d(1, 1);
	//glVertex3d(0.5, 0.5, 0);
	//glTexCoord2d(1, 0);
	//glVertex3d(0.5, -0.5, 0);
	//glTexCoord2d(0, 0);
	//glVertex3d(-0.5, -0.5, 0);
	//glTexCoord2d(0, 1);
	//glVertex3d(-0.5, 0.5, 0);
	//glEnd();


	//glPopMatrix();







	////Квадратик с освещением
	//phong_sh.UseShader();

	//float light_pos[4] = { light.x(),light.y(), light.z(), 1 };
	//float light_pos_v[4];
	//
	////переносим координаты света в видовые координаты
	//MatrixMultiply<float, 1, 4, 4, 4>(light_pos, view_matrix, light_pos_v);

	//
	//location = glGetUniformLocationARB(phong_sh.program, "Ia");
	//glUniform3fARB(location, 1, 1, 1);
	//location = glGetUniformLocationARB(phong_sh.program, "Id");
	//glUniform3fARB(location, 1, 1, 1);
	//location = glGetUniformLocationARB(phong_sh.program, "Is");
	//glUniform3fARB(location, 1, 1, 1);

	//location = glGetUniformLocationARB(phong_sh.program, "ma");
	//glUniform3fARB(location, 0.1, 0.1, 0.1);
	//location = glGetUniformLocationARB(phong_sh.program, "md");
	//glUniform3fARB(location, 0.6, 0.6, 0.6);
	//location = glGetUniformLocationARB(phong_sh.program, "ms");
	//glUniform4fARB(location, 0, 1, 0, 300);
	//	
	//
	//location = glGetUniformLocationARB(phong_sh.program, "light_pos_v");
	//glUniform3fvARB(location,1,light_pos_v);
	//glPushMatrix();

	//glTranslated(0, 0, 0);

	//glBegin(GL_QUADS);
	//glNormal3d(0, 0, 1);
	//glTexCoord2d(1, 1);
	//glVertex3d(0.5, 0.5, 0);
	//glTexCoord2d(1, 0);
	//glVertex3d(0.5, -0.5, 0);
	//glTexCoord2d(0, 0);
	//glVertex3d(-0.5, -0.5, 0);
	//glTexCoord2d(0, 1);
	//glVertex3d(-0.5, 0.5, 0);
	//glEnd();


	//glPopMatrix();



	//Квадратик без освещения

	if (!win)
		line_tex.Bind();
	else
	{
		cong_tex.Bind();
	}
	colorLight(1, 1, 1, 1, 1, 1, 1, 1);
	glPushMatrix();

	glTranslated(12, 0, 0);


	glBegin(GL_QUADS);
	glNormal3d(0, 0, 1);
	glTexCoord2d(1, 1);
	glVertex3d(4, 4, 0);
	glTexCoord2d(1, 0);
	glVertex3d(4, -4, 0);
	glTexCoord2d(0, 0);
	glVertex3d(-4, -4, 0);
	glTexCoord2d(0, 1);
	glVertex3d(-4, 4, 0);
	glEnd();


	glPopMatrix();


	//квадратик с ВБ


	vb_sh.UseShader();

	glActiveTexture(GL_TEXTURE0);
	stankin_tex.Bind();
	glActiveTexture(GL_TEXTURE1);
	vb_tex.Bind();

	location = glGetUniformLocationARB(vb_sh.program, "time");
	glUniform1fARB(location, full_time);
	location = glGetUniformLocationARB(vb_sh.program, "tex_stankin");
	glUniform1iARB(location, 0);
	location = glGetUniformLocationARB(vb_sh.program, "tex_vb");
	glUniform1iARB(location, 1);

	glPushMatrix();
		glBegin(GL_QUADS);
		glNormal3d(0, 0, 2);
		glTexCoord2d(1, 1);
		glVertex3d(1, 1, 0);
		glTexCoord2d(1, 0);
		glVertex3d(1, -1, 0);
		glTexCoord2d(0, 0);
		glVertex3d(-1, -1, 0);
		glTexCoord2d(0, 1);
		glVertex3d(-1, 1, 0);
	glEnd();


	glPopMatrix();


	////обезьянка без шейдеров
	//glPushMatrix();
	//Shader::DontUseShaders();
	//glActiveTexture(GL_TEXTURE0);
	//monkey_tex.Bind();
	//glShadeModel(GL_SMOOTH);
	//glTranslated(-1, 0, 0.5);
	//glScaled(0.1, 0.1, 0.1);
	//glRotated(180, 0, 0, 1);
	//f.Draw();
	//glPopMatrix();

	////обезьянка с шейдерами


	//simple_texture_sh.UseShader();
	//location = glGetUniformLocationARB(simple_texture_sh.program, "tex");
	//glUniform1iARB(location, 0);
	//glActiveTexture(GL_TEXTURE0);
	//monkey_tex.Bind();


	//glPushMatrix();
	//glTranslated(-1, 1, 0.5);
	//glScaled(0.1, 0.1, 0.1);
	//glRotated(180, 0, 0, 1);
	//f.Draw();
	//glPopMatrix();

	//===============================================
	
	
	//сбрасываем все трансформации
	glLoadIdentity();
	camera.SetUpCamera();	
	Shader::DontUseShaders();
	//рисуем источник света
	light.DrawLightGizmo();

	//================Сообщение в верхнем левом углу=======================
	glActiveTexture(GL_TEXTURE0);
	//переключаемся на матрицу проекции
	glMatrixMode(GL_PROJECTION);
	//сохраняем текущую матрицу проекции с перспективным преобразованием
	glPushMatrix();
	//загружаем единичную матрицу в матрицу проекции
	glLoadIdentity();

	//устанавливаем матрицу паралельной проекции
	glOrtho(0, gl.getWidth() - 1, 0, gl.getHeight() - 1, 0, 1);

	//переключаемся на моделвью матрицу
	glMatrixMode(GL_MODELVIEW);
	//сохраняем матрицу
	glPushMatrix();
    //сбразываем все трансформации и настройки камеры загрузкой единичной матрицы
	glLoadIdentity();

	//отрисованное тут будет визуалзироватся в 2д системе координат
	//нижний левый угол окна - точка (0,0)
	//верхний правый угол (ширина_окна - 1, высота_окна - 1)

	
	std::wstringstream ss;
	ss << std::fixed << std::setprecision(3);
	ss << "T - " << (texturing ? L"[вкл]выкл  " : L" вкл[выкл] ") << L"текстур" << std::endl;
	ss << "L - " << (lightning ? L"[вкл]выкл  " : L" вкл[выкл] ") << L"освещение" << std::endl;
	ss << "A - " << (alpha ? L"[вкл]выкл  " : L" вкл[выкл] ") << L"альфа-наложение" << std::endl;
	ss << L"F - Свет из камеры" << std::endl;
	ss << L"G - двигать свет по горизонтали" << std::endl;
	ss << L"G+ЛКМ двигать свет по вертекали" << std::endl;
	/*ss << L"Коорд. света: (" << std::setw(7) <<  light.x() << "," << std::setw(7) << light.y() << "," << std::setw(7) << light.z() << ")" << std::endl;
	ss << L"Коорд. камеры: (" << std::setw(7) << camera.x() << "," << std::setw(7) << camera.y() << "," << std::setw(7) << camera.z() << ")" << std::endl;*/
	//ss << L"Параметры камеры: R=" << std::setw(7) << camera.distance() << ",fi1=" << std::setw(7) << camera.fi1() << ",fi2=" << std::setw(7) << camera.fi2() << std::endl;
	/*ss << L"delta_time: " << std::setprecision(5)<< delta_time << std::endl;
	ss << L"full_time: " << std::setprecision(2) << full_time << std::endl;*/
	ss << L"Выбран фрагмент: " << std::setprecision(0) << fl << std::endl;
	ss << L"Чтобы двигать фрагмент, используйте Q и E" << std::endl;
	ss << L"Чтобы вращать фрагмент, используйте W и S" << std::endl;
	ss << L"нужные позиции у фрагментов: 1: " << first_done << " 2:" << second_done << " 3:" << third_done << " 4:" << fourth_done << std::endl;
	if (!win) {
		ss << rubbish1_done << rubbish2_done << win << std::endl;
	}
	else
	{
		ss << L"Вы победили!!!" << std::endl;
	}
	
	text.setPosition(10, gl.getHeight() - 10 - 180);
	text.setText(ss.str().c_str());
	
	text.Draw();

	//восстанавливаем матрицу проекции на перспективу, которую сохраняли ранее.
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();
	
	
}   




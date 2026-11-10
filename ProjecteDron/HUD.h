#pragma once
#include "Shader.h"
#include "Texture.h"
#include "Camera.h"
#include "Dron.h"

class HUD 
{
public:
	HUD();

	void draw(Camera& camera);
	void update(Dron* dron); // Actualitza els elements del HUD

private:
	Shader m_shader; // Shader per al HUD?
	Texture* m_texturaFont; // Textura per al HUD?

	GLuint VAO, VBO; // fa falta???
	
	Dron* m_dron; // Referència al dron per obtenir dades
};


void Occlusion_Query()
{
	//=================
	// Render: Quad Texture
	//=================
	glUseProgram(program2);
	glBindVertexArray(vaoQuad);

	glGenQueriesARB(1, &m_query);

	for (int i = 0; i < 1; i++)
	{
		glBeginQueryARB(GL_SAMPLES_PASSED_ARB, m_query);
		//glUniform1i(detIDLoc, i+1);
		glUniform1f(detMinLoc, (float)(i + 1) / 256.0);
		glUniform1f(detMaxLoc, (float)(i + 2) / 256.0);
		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
		glEndQueryARB(GL_SAMPLES_PASSED_ARB);
		//do { glGetQueryObjectivARB(m_query, GL_QUERY_RESULT_AVAILABLE_ARB, &available);
		//} while (!available);

		glGetQueryObjectuivARB(m_query, GL_QUERY_RESULT_ARB, &m_count[i]);

		//glutSwapBuffers();
		//glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // clear detectors before textured quad
	}
	glDeleteQueriesARB(1, &m_query);
}


glm::mat4 lookat(GLdouble eyex, GLdouble eyey, GLdouble eyez)  // camera
{
	// https://www.opengl.org/discussion_boards/showthread.php/153269-glulookat-replacement
	glm::vec3 forward(-eyex, -eyey, -eyez), side, up(0.0, 1.0, 0.0);
	glm::mat4 m;

	forward = normalize(forward);
	side = cross(forward, up); // side = forward x up
	side = normalize(side);
	up = cross(side, forward); // Now: up = side x forward

	m[0][0] = side[0];
	m[1][0] = side[1];
	m[2][0] = side[2];
	m[3][0] = 0;

	m[0][1] = up[0];
	m[1][1] = up[1];
	m[2][1] = up[2];
	m[3][1] = 0;

	m[0][2] = -forward[0];
	m[1][2] = -forward[1];
	m[2][2] = -forward[2];
	m[3][2] = 0;

	m[0][3] = 0;
	m[1][3] = 0;
	m[2][3] = 0;
	m[3][3] = 1.0;

	return(m);
	//return(glm::translate(m, glm::vec3(-eyex,-eyey,-eyez)));
}

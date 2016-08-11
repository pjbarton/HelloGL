#version 430 core

in vec4 fragmentColor;
out vec4 color;

void main()
{
	color = fragmentColor;

	//color = fragmentColor / 1.2 + 0.16;

	//color = vec4(128.0/256, 0.0, 0.0, 1.0);

	if (false)
	{
		if (fragmentColor.g <= 64/256.0 && fragmentColor.g > 63/256.0 ||
			fragmentColor.r <= 64/256.0 && fragmentColor.r > 63/256.0 ||
			fragmentColor.b <= 64/256.0 && fragmentColor.b > 63/256.0)
			color = fragmentColor;
		else
			discard;
			//color = vec4(0.0, 0.0, 0.0, 1.0);
	}

	if (false)
	{
		for (int i = 0; i < 192; i++)
		{
			//if (fragmentColor.g <= (i+1) / 256.0 && fragmentColor.g > i / 256.0)
				color = vec4(i/256.0, 0.0,0.0,1.0);
		}
	}

}
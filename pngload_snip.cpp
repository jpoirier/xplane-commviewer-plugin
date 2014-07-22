
#define INVALID_TEXTURE_ID	-1
#define MAX_TEXTURES 100


typedef struct {
    unsigned char* data;
    png_uint_32	width;
    png_uint_32	height;
    png_uint_32	channels;
    png_uint_32 pad;
} ImageInfo;

static XPLMTextureID texture[MAX_TEXTURES];

static int CurTextureId;

int LoadGL_PNG_Textures(char* textureName) {
    int status = INVALID_TEXTURE_ID;
    ImageInfo imageInfo;
    pngInfo info;  // has width, depth, height, Alpha

    if (CurTextureId < MAX_TEXTURES) {
        // Populates texture[i] with a unique X-Plane-generated texture ID
        XPLMGenerateTextureNumbers(&texture[CurTextureId], 1);
        glBindTexture(GL_TEXTURE_2D, texture[CurTextureId]);

        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL);

        if (pngLoad(textureName, PNG_NOMIPMAP, PNG_ALPHA, &info))  {
            status = CurTextureId;
            CurTextureId++;
        }
    }

    return status;
}


To draw a texture, I use this function... you have to fill in your own texture coordinate and position details:


void DrawPNGtexture (TEXTURE_ID textId, .... also pass in draw coordinates and texture coordinates)
{
    // XPLMSetGraphicsState(0/*Fog*/, 1/*TexUnits*/, 0/*Lighting*/, 0/*AlphaTesting*/, 1/*AlphaBlending*/, 1/*DepthTesting*/, 0/*DepthWriting*/)
    XPLMSetGraphicsState(0, 1, 0, 0, 1, 1, 0);

    glDepthFunc(GL_LEQUAL);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glColor3f(1, 1, 1);

    XPLMBindTexture2d(texture[textId], 0);

    setScissorArea();	 // this function uses glScissor command to set clipping boundaries
    glEnable (GL_SCISSOR_TEST);

    glBegin(GL_QUADS);
        // draw png texture in this section using 4 glTexCoord2f function calls ... textureCoords and drawCoords are my own coordinates class
        glTexCoord2f(textureCoords.get_xLeft(), textureCoords.get_yBottom());
        glVertex2f(drawCoords.get_xLeft(), drawCoords.get_yBottom()));	// Bottom Left Of The Texture and Quad
        glTexCoord2f(textureCoords.get_xRight(), textureCoords.get_yBottom());
        glVertex2f(drawCoords.get_xRight(), drawCoords.get_yBottom()));	// Bottom Right Of The Texture and Quad
        glTexCoord2f(textureCoords.get_xRight(), textureCoords.get_yTop());
        glVertex2f(drawCoords.get_xRight(), drawCoords.get_yTop()));	// Top Right Of The Texture and Quad
        glTexCoord2f(textureCoords.get_xLeft(), textureCoords.get_yTop());
        glVertex2f(drawCoords.get_xLeft(), drawCoords.get_yTop()));	// Top Left Of The Texture and Quad
    glEnd();

    glDisable (GL_SCISSOR_TEST);
}

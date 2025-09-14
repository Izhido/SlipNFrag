#include "AppState.h"
#include "PerFrame.h" // This header is specified second in the list to allow headers in AppState.h to include the core engine structs
#include "Utils.h"
#if !defined(NDEBUG) || defined(ENABLE_DEBUG_UTILS)
#include "RendererNames.h"
#endif

float PerFrame::GammaCorrect(float component)
{
	if (std::isnan(component) || component <= 0)
	{
		return 0;
	}
	else if (component >= 1)
	{
		return 1;
	}
	else if (component < 0.04045)
	{
		return component / 12.92;
	}
	return std::pow((component + 0.055f) / 1.055f, 2.4f);
}

unsigned char PerFrame::AveragePixels(std::vector<unsigned char>& pixdata)
{
	int        r,g,b;
	int        i;
	int        vis;
	int        pix;
	int        dr, dg, db;
	int        bestdistortion, distortion;
	int        bestcolor;
	unsigned char *pal;
	int        e;

	vis = 0;
	r = g = b = 0;
	for (i=0 ; i<pixdata.size() ; i++)
	{
		pix = pixdata[i];
		if (pix == 255)
		{
			continue;
		}
		else if (pix >= 224)
		{
			return pix;
		}

		r += host_basepal[pix*3];
		g += host_basepal[pix*3+1];
		b += host_basepal[pix*3+2];
		vis++;
	}

	if (vis == 0)
		return 255;

	r /= vis;
	g /= vis;
	b /= vis;

//
// find the best color
//
	bestdistortion = r*r + g*g + b*b;
	bestcolor = 0;
	i = 0;
	e = 224;

	for ( ; i< e ; i++)
	{
		pix = i;    //pixdata[i];

		pal = host_basepal.data() + pix*3;

		dr = r - (int)pal[0];
		dg = g - (int)pal[1];
		db = b - (int)pal[2];

		distortion = dr*dr + dg*dg + db*db;
		if (distortion < bestdistortion)
		{
			if (!distortion)
			{
				return pix;        // perfect match
			}

			bestdistortion = distortion;
			bestcolor = pix;
		}
	}

	return bestcolor;
}

void PerFrame::GenerateMipmaps(Buffer* stagingBuffer, VkDeviceSize offset, LoadedSharedMemoryTexture* loadedTexture)
{
	unsigned char* source = nullptr;
	int sourceMipWidth = 0;
	int sourceMipHeight = 0;
	auto target = ((unsigned char*)stagingBuffer->mapped) + offset;
	auto mipWidth = loadedTexture->texture->width;
	auto mipHeight = loadedTexture->texture->height;
	auto mips = loadedTexture->mips;
	while (mips > 0)
	{
		source = target;
		sourceMipWidth = mipWidth;
		sourceMipHeight = mipHeight;
		target += (mipWidth * mipHeight);
		mipWidth /= 2;
		if (mipWidth < 1)
		{
			mipWidth = 1;
		}
		mipHeight /= 2;
		if (mipHeight < 1)
		{
			mipHeight = 1;
		}
		mips--;
	}
	std::vector<unsigned char> pixdata;
	auto mipLevels = (int)(std::floor(std::log2(std::max(sourceMipWidth, sourceMipHeight)))) + 1;
	for (auto miplevel = 1 ; miplevel<mipLevels ; miplevel++)
	{
		auto mipstepx = std::min(1<<miplevel, sourceMipWidth);
		auto mipstepy = std::min(1<<miplevel, sourceMipHeight);
		for (auto y=0 ; y<sourceMipHeight ; y+=mipstepy)
		{
			for (auto x = 0 ; x<sourceMipWidth ; x+= mipstepx)
			{
				pixdata.clear();
				for (auto yy=0 ; yy<mipstepy ; yy++)
					for (auto xx=0 ; xx<mipstepx ; xx++)
					{
						pixdata.push_back(source[ (y+yy)*sourceMipWidth + x + xx ]);
					}
				*target++ = AveragePixels (pixdata);
			}
		}
	}
}

void PerFrame::LoadStagingBuffer(AppState& appState, Buffer* stagingBuffer)
{
    VkDeviceSize offset = 0;
	auto loadedAliasIndexBuffer = appState.Scene.indexBuffers.firstAliasIndices8;
	while (loadedAliasIndexBuffer != nullptr)
	{
		auto target = ((unsigned char*)stagingBuffer->mapped) + offset;
		auto aliashdr = (aliashdr_t *)loadedAliasIndexBuffer->source;
		auto mdl = (mdl_t *)((byte *)aliashdr + aliashdr->model);
		auto triangle = (mtriangle_t *)((byte *)aliashdr + aliashdr->triangles);
		auto stverts = (stvert_t *)((byte *)aliashdr + aliashdr->stverts);
		for (auto i = 0; i < mdl->numtris; i++)
		{
			auto v0 = triangle->vertindex[0];
			auto v1 = triangle->vertindex[1];
			auto v2 = triangle->vertindex[2];
			auto v0back = (((stverts[v0].onseam & ALIAS_ONSEAM) == ALIAS_ONSEAM) && triangle->facesfront == 0);
			auto v1back = (((stverts[v1].onseam & ALIAS_ONSEAM) == ALIAS_ONSEAM) && triangle->facesfront == 0);
			auto v2back = (((stverts[v2].onseam & ALIAS_ONSEAM) == ALIAS_ONSEAM) && triangle->facesfront == 0);
			*target++ = v0 * 2 + (v0back ? 1 : 0);
			*target++ = v1 * 2 + (v1back ? 1 : 0);
			*target++ = v2 * 2 + (v2back ? 1 : 0);
			triangle++;
		}
		offset += loadedAliasIndexBuffer->size;
		loadedAliasIndexBuffer = loadedAliasIndexBuffer->next;
	}
	while (offset % 2 != 0)
	{
		offset++;
	}
	loadedAliasIndexBuffer = appState.Scene.indexBuffers.firstAliasIndices16;
	while (loadedAliasIndexBuffer != nullptr)
	{
		auto target = (uint16_t*)(((unsigned char*)stagingBuffer->mapped) + offset);
		auto aliashdr = (aliashdr_t *)loadedAliasIndexBuffer->source;
		auto mdl = (mdl_t *)((byte *)aliashdr + aliashdr->model);
		auto triangle = (mtriangle_t *)((byte *)aliashdr + aliashdr->triangles);
		auto stverts = (stvert_t *)((byte *)aliashdr + aliashdr->stverts);
		for (auto i = 0; i < mdl->numtris; i++)
		{
			auto v0 = triangle->vertindex[0];
			auto v1 = triangle->vertindex[1];
			auto v2 = triangle->vertindex[2];
			auto v0back = (((stverts[v0].onseam & ALIAS_ONSEAM) == ALIAS_ONSEAM) && triangle->facesfront == 0);
			auto v1back = (((stverts[v1].onseam & ALIAS_ONSEAM) == ALIAS_ONSEAM) && triangle->facesfront == 0);
			auto v2back = (((stverts[v2].onseam & ALIAS_ONSEAM) == ALIAS_ONSEAM) && triangle->facesfront == 0);
			*target++ = v0 * 2 + (v0back ? 1 : 0);
			*target++ = v1 * 2 + (v1back ? 1 : 0);
			*target++ = v2 * 2 + (v2back ? 1 : 0);
			triangle++;
		}
		offset += loadedAliasIndexBuffer->size;
		loadedAliasIndexBuffer = loadedAliasIndexBuffer->next;
	}
	while (offset % 4 != 0)
	{
		offset++;
	}
	loadedAliasIndexBuffer = appState.Scene.indexBuffers.firstAliasIndices32;
	while (loadedAliasIndexBuffer != nullptr)
	{
		auto target = (uint32_t*)(((unsigned char*)stagingBuffer->mapped) + offset);
		auto aliashdr = (aliashdr_t *)loadedAliasIndexBuffer->source;
		auto mdl = (mdl_t *)((byte *)aliashdr + aliashdr->model);
		auto triangle = (mtriangle_t *)((byte *)aliashdr + aliashdr->triangles);
		auto stverts = (stvert_t *)((byte *)aliashdr + aliashdr->stverts);
		for (auto i = 0; i < mdl->numtris; i++)
		{
			auto v0 = triangle->vertindex[0];
			auto v1 = triangle->vertindex[1];
			auto v2 = triangle->vertindex[2];
			auto v0back = (((stverts[v0].onseam & ALIAS_ONSEAM) == ALIAS_ONSEAM) && triangle->facesfront == 0);
			auto v1back = (((stverts[v1].onseam & ALIAS_ONSEAM) == ALIAS_ONSEAM) && triangle->facesfront == 0);
			auto v2back = (((stverts[v2].onseam & ALIAS_ONSEAM) == ALIAS_ONSEAM) && triangle->facesfront == 0);
			*target++ = v0 * 2 + (v0back ? 1 : 0);
			*target++ = v1 * 2 + (v1back ? 1 : 0);
			*target++ = v2 * 2 + (v2back ? 1 : 0);
			triangle++;
		}
		offset += loadedAliasIndexBuffer->size;
		loadedAliasIndexBuffer = loadedAliasIndexBuffer->next;
	}
	auto loadedBuffer = appState.Scene.aliasBuffers.firstAliasVertices;
	while (loadedBuffer != nullptr)
	{
		auto target = (byte*)(((unsigned char*)stagingBuffer->mapped) + offset);
		auto source = (trivertx_t*)loadedBuffer->source;
		for (auto i = 0; i < loadedBuffer->size; i += 2 * 3 * sizeof(byte))
		{
			auto x = source->v[0];
			auto y = source->v[1];
			auto z = source->v[2];
			*target++ = x;
			*target++ = y;
			*target++ = z;
			*target++ = x;
			*target++ = y;
			*target++ = z;
			source++;
		}
		offset += loadedBuffer->size;
		loadedBuffer = loadedBuffer->next;
	}
	auto loadedTexCoordsBuffer = appState.Scene.aliasBuffers.firstAliasTexCoords;
	while (loadedTexCoordsBuffer != nullptr)
	{
		auto target = (float*)(((unsigned char*)stagingBuffer->mapped) + offset);
		auto source = (stvert_t*)loadedTexCoordsBuffer->source;
		for (auto i = 0; i < loadedTexCoordsBuffer->size; i += 2 * 2 * sizeof(float))
		{
			auto s = (float)(source->s >> 16);
			auto t = (float)(source->t >> 16);
			s /= loadedTexCoordsBuffer->width;
			t /= loadedTexCoordsBuffer->height;
			*target++ = s;
			*target++ = t;
			*target++ = s + 0.5f;
			*target++ = t;
			source++;
		}
		offset += loadedTexCoordsBuffer->size;
		loadedTexCoordsBuffer = loadedTexCoordsBuffer->next;
	}
	if (appState.Scene.verticesSize > 0)
	{
		if (appState.Scene.floorVerticesSize > 0)
		{
			auto target = (float*)(((unsigned char*)stagingBuffer->mapped) + offset);
			*target++ = -0.5;
			*target++ = appState.DistanceToFloor;
			*target++ = -0.5;
			*target++ = 0.5;
			*target++ = appState.DistanceToFloor;
			*target++ = -0.5;
			*target++ = 0.5;
			*target++ = appState.DistanceToFloor;
			*target++ = 0.5;
			*target++ = -0.5;
			*target++ = appState.DistanceToFloor;
			*target++ = 0.5;
			offset += appState.Scene.floorVerticesSize;
		}
		controllerVertexBase = appState.Scene.floorVerticesSize;
		if (appState.Scene.controllerVerticesSize > 0)
		{
			auto target = (float*)(((unsigned char*)stagingBuffer->mapped) + offset);
			if (appState.LeftController.PoseIsValid)
			{
				target = appState.LeftController.WriteVertices(target);
			}
			if (appState.RightController.PoseIsValid)
			{
				target = appState.RightController.WriteVertices(target);
			}
			offset += appState.Scene.controllerVerticesSize;
		}
		skyVertexBase = controllerVertexBase + appState.Scene.controllerVerticesSize;
        if (appState.Scene.lastSky >= 0 || appState.Scene.lastSkyRGBA >= 0)
        {
			auto firstVertex = (appState.Scene.lastSky >= 0 ? appState.Scene.loadedSky.firstVertex : appState.Scene.loadedSkyRGBA.firstVertex);
            auto source = d_lists.textured_vertices.data() + firstVertex * 3;
            auto target = (float*)(((unsigned char*)stagingBuffer->mapped) + offset);
            *target++ = -appState.SkyLeft + appState.SkyHorizontal * source[0];
			*target++ = appState.SkyTop - appState.SkyVertical * source[1];
			*target++ = -source[2];
			*target++ = -appState.SkyLeft + appState.SkyHorizontal * source[3];
			*target++ = appState.SkyTop - appState.SkyVertical * source[4];
			*target++ = -source[5];
			*target++ = -appState.SkyLeft + appState.SkyHorizontal * source[6];
			*target++ = appState.SkyTop - appState.SkyVertical * source[7];
			*target++ = -source[8];
			*target++ = -appState.SkyLeft + appState.SkyHorizontal * source[9];
			*target++ = appState.SkyTop - appState.SkyVertical * source[10];
			*target++ = -source[11];
        }
		offset += appState.Scene.skyVerticesSize;
		auto count = (size_t)appState.Scene.coloredVerticesSize / sizeof(float);
		std::copy(d_lists.colored_vertices.data(), d_lists.colored_vertices.data() + count, (float*)(((unsigned char*)stagingBuffer->mapped) + offset));
		offset += appState.Scene.coloredVerticesSize;
		count = (size_t)appState.Scene.cutoutVerticesSize / sizeof(float);
		std::copy(d_lists.cutout_vertices.data(), d_lists.cutout_vertices.data() + count, (float*)(((unsigned char*)stagingBuffer->mapped) + offset));
		offset += appState.Scene.cutoutVerticesSize;
	}
	if (appState.Scene.floorAttributesSize > 0)
	{
		auto target = (float*)(((unsigned char*)stagingBuffer->mapped) + offset);
		*target++ = 0;
		*target++ = 0;
		*target++ = 1;
		*target++ = 0;
		*target++ = 1;
		*target++ = 1;
		*target++ = 0;
		*target++ = 1;
		offset += appState.Scene.floorAttributesSize;
	}
	controllerAttributeBase = appState.Scene.floorAttributesSize;
	if (appState.Scene.controllerAttributesSize > 0)
	{
		auto target = (float*)(((unsigned char*)stagingBuffer->mapped) + offset);
		if (appState.LeftController.PoseIsValid)
		{
			target = Controller::WriteAttributes(target);
		}
		if (appState.RightController.PoseIsValid)
		{
			target = Controller::WriteAttributes(target);
		}
		offset += appState.Scene.controllerAttributesSize;
	}
	skyAttributeBase = controllerAttributeBase + appState.Scene.controllerAttributesSize;
    if (appState.Scene.lastSky >= 0 || appState.Scene.lastSkyRGBA >= 0)
    {
        float skyTexCoordsLeft = 0.5f - appState.SkyLeft / 2;
        float skyTexCoordsTop = 0.5f - appState.SkyTop / 2;
        float skyTexCoordsHorizontal = appState.SkyHorizontal / 2;
        float skyTexCoordsVertical = appState.SkyVertical / 2;
		auto firstVertex = (appState.Scene.lastSky >= 0 ? appState.Scene.loadedSky.firstVertex : appState.Scene.loadedSkyRGBA.firstVertex);
        auto source = d_lists.textured_attributes.data() + firstVertex * 2;
        auto target = (float*)(((unsigned char*)stagingBuffer->mapped) + offset);
        *target++ = skyTexCoordsLeft + skyTexCoordsHorizontal * source[0];
		*target++ = skyTexCoordsTop + skyTexCoordsVertical * source[1];
		*target++ = skyTexCoordsLeft + skyTexCoordsHorizontal * source[2];
		*target++ = skyTexCoordsTop + skyTexCoordsVertical * source[3];
		*target++ = skyTexCoordsLeft + skyTexCoordsHorizontal * source[4];
		*target++ = skyTexCoordsTop + skyTexCoordsVertical * source[5];
		*target++ = skyTexCoordsLeft + skyTexCoordsHorizontal * source[6];
		*target++ = skyTexCoordsTop + skyTexCoordsVertical * source[7];
    }
	offset += appState.Scene.skyAttributesSize;
	aliasAttributeBase = skyAttributeBase + appState.Scene.skyAttributesSize;
	auto count = (size_t)appState.Scene.aliasAttributesSize / sizeof(float);
	std::copy(d_lists.alias_attributes.data(), d_lists.alias_attributes.data() + count, (float*)(((unsigned char*)stagingBuffer->mapped) + offset));
	offset += appState.Scene.aliasAttributesSize;
	count = (size_t)appState.Scene.coloredColorsSize / sizeof(float);
	std::copy(d_lists.colored_colors.data(), d_lists.colored_colors.data() + count, (float*)(((unsigned char*)stagingBuffer->mapped) + offset));
	offset += appState.Scene.coloredColorsSize;
	if (appState.IndexTypeUInt8Enabled)
	{
		if (appState.Scene.indices8Size > 0)
		{
			if (appState.Scene.floorIndicesSize > 0)
			{
				auto target = ((unsigned char*)stagingBuffer->mapped) + offset;
				*target++ = 0;
				*target++ = 1;
				*target++ = 3;
				*target++ = 2;
				offset += appState.Scene.floorIndicesSize;
			}
			controllerIndexBase = appState.Scene.floorIndicesSize;
			if (appState.Scene.controllerIndicesSize > 0)
			{
				auto target = ((unsigned char*)stagingBuffer->mapped) + offset;
				auto controllerOffset = 0;
				if (appState.LeftController.PoseIsValid)
				{
					target = Controller::WriteIndices8(target, controllerOffset);
					controllerOffset += 8;
					target = Controller::WriteIndices8(target, controllerOffset);
					controllerOffset += 8;
				}
				if (appState.RightController.PoseIsValid)
				{
					target = Controller::WriteIndices8(target, controllerOffset);
					controllerOffset += 8;
					target = Controller::WriteIndices8(target, controllerOffset);
				}
				offset += appState.Scene.controllerIndicesSize;
			}
			coloredIndex8Base = controllerIndexBase + appState.Scene.controllerIndicesSize;
			memcpy(((unsigned char*)stagingBuffer->mapped) + offset, d_lists.colored_indices8.data(), appState.Scene.coloredIndices8Size);
			offset += appState.Scene.coloredIndices8Size;
			cutoutIndex8Base = coloredIndex8Base + appState.Scene.coloredIndices8Size;
			memcpy(((unsigned char*)stagingBuffer->mapped) + offset, d_lists.cutout_indices8.data(), appState.Scene.cutoutIndices8Size);
			offset += appState.Scene.cutoutIndices8Size;
			while (offset % 4 != 0)
			{
				offset++;
			}
		}
		if (appState.Scene.indices16Size > 0)
		{
			coloredIndex16Base = 0;
			memcpy(((unsigned char*)stagingBuffer->mapped) + offset, d_lists.colored_indices16.data(), appState.Scene.coloredIndices16Size);
			offset += appState.Scene.coloredIndices16Size;
			cutoutIndex16Base = coloredIndex16Base + appState.Scene.coloredIndices16Size;
			memcpy(((unsigned char*)stagingBuffer->mapped) + offset, d_lists.cutout_indices16.data(), appState.Scene.cutoutIndices16Size);
			offset += appState.Scene.cutoutIndices16Size;
			while (offset % 4 != 0)
			{
				offset++;
			}
		}
	}
	else if (appState.Scene.indices16Size > 0)
	{
		if (appState.Scene.floorIndicesSize > 0)
		{
			auto target = ((uint16_t*)stagingBuffer->mapped) + offset / sizeof(uint16_t);
			*target++ = 0;
			*target++ = 1;
			*target++ = 3;
			*target++ = 2;
			offset += appState.Scene.floorIndicesSize;
		}
		controllerIndexBase = appState.Scene.floorIndicesSize;
		if (appState.Scene.controllerIndicesSize > 0)
		{
			auto target = ((uint16_t*)stagingBuffer->mapped) + offset / sizeof(uint16_t);
			auto controllerOffset = 0;
			if (appState.LeftController.PoseIsValid)
			{
				target = Controller::WriteIndices16(target, controllerOffset);
				controllerOffset += 8;
				target = Controller::WriteIndices16(target, controllerOffset);
				controllerOffset += 8;
			}
			if (appState.RightController.PoseIsValid)
			{
				target = Controller::WriteIndices16(target, controllerOffset);
				controllerOffset += 8;
				target = Controller::WriteIndices16(target, controllerOffset);
			}
			offset += appState.Scene.controllerIndicesSize;
		}
		coloredIndex16Base = controllerIndexBase + appState.Scene.controllerIndicesSize;
		auto target = ((uint16_t*)stagingBuffer->mapped) + offset / sizeof(uint16_t);
		for (auto i = 0; i < appState.Scene.coloredIndices8Size; i++)
		{
			*target++ = d_lists.colored_indices8[i];
		}
		offset += appState.Scene.coloredIndices8Size * sizeof(uint16_t);
		memcpy(((unsigned char*)stagingBuffer->mapped) + offset, d_lists.colored_indices16.data(), appState.Scene.coloredIndices16Size);
		offset += appState.Scene.coloredIndices16Size;
		cutoutIndex16Base = coloredIndex16Base + appState.Scene.coloredIndices8Size * sizeof(uint16_t) + appState.Scene.coloredIndices16Size;
		target = ((uint16_t*)stagingBuffer->mapped) + offset / sizeof(uint16_t);
		for (auto i = 0; i < appState.Scene.cutoutIndices8Size; i++)
		{
			*target++ = d_lists.cutout_indices8[i];
		}
		offset += appState.Scene.cutoutIndices8Size * sizeof(uint16_t);
		memcpy(((unsigned char*)stagingBuffer->mapped) + offset, d_lists.cutout_indices16.data(), appState.Scene.cutoutIndices16Size);
		offset += appState.Scene.cutoutIndices16Size;
		while (offset % 4 != 0)
		{
			offset++;
		}
	}
	count = (size_t)appState.Scene.coloredIndices32Size / sizeof(uint32_t);
	std::copy((uint32_t*)d_lists.colored_indices32.data(), (uint32_t*)d_lists.colored_indices32.data() + count, (uint32_t*)(((unsigned char*)stagingBuffer->mapped) + offset));
	offset += appState.Scene.coloredIndices32Size;
	cutoutIndex32Base = appState.Scene.coloredIndices32Size;
	count = (size_t)appState.Scene.cutoutIndices32Size / sizeof(uint32_t);
	std::copy((uint32_t*)d_lists.cutout_indices32.data(), (uint32_t*)d_lists.cutout_indices32.data() + count, (uint32_t*)(((unsigned char*)stagingBuffer->mapped) + offset));
	offset += appState.Scene.cutoutIndices32Size;


	for (auto& chain : appState.Scene.lightmapChains)
	{
		uint32_t* delayedCopyBuffer = nullptr;
		size_t delayedCopySize = 0;
		auto loaded = chain.first;
		while (loaded != nullptr)
		{
			count = (size_t)loaded->size / sizeof(uint32_t);
			if (delayedCopyBuffer == nullptr)
			{
				delayedCopyBuffer = (uint32_t*)loaded->source;
				delayedCopySize = count;
			}
			else if (delayedCopyBuffer + delayedCopySize == (uint32_t*)loaded->source)
			{
				delayedCopySize += count;
			}
			else
			{
				std::copy(delayedCopyBuffer, delayedCopyBuffer + delayedCopySize, (uint32_t*)(((unsigned char*)stagingBuffer->mapped) + offset));
				offset += delayedCopySize * sizeof(uint32_t);
				delayedCopyBuffer = (uint32_t*)loaded->source;
				delayedCopySize = count;
			}
			loaded = loaded->next;
		}
		if (delayedCopyBuffer != nullptr)
		{
			std::copy(delayedCopyBuffer, delayedCopyBuffer + delayedCopySize, (uint32_t*)(((unsigned char*)stagingBuffer->mapped) + offset));
			offset += delayedCopySize * sizeof(uint32_t);
		}
	}

	for (auto& chain : appState.Scene.lightmapRGBChains)
	{
		uint32_t* delayedCopyBuffer = nullptr;
		size_t delayedCopySize = 0;
		auto loaded = chain.first;
		while (loaded != nullptr)
		{
			count = (size_t)loaded->size / sizeof(uint32_t);
			if (delayedCopyBuffer == nullptr)
			{
				delayedCopyBuffer = (uint32_t*)loaded->source;
				delayedCopySize = count;
			}
			else if (delayedCopyBuffer + delayedCopySize == (uint32_t*)loaded->source)
			{
				delayedCopySize += count;
			}
			else
			{
				std::copy(delayedCopyBuffer, delayedCopyBuffer + delayedCopySize, (uint32_t*)(((unsigned char*)stagingBuffer->mapped) + offset));
				offset += delayedCopySize * sizeof(uint32_t);
				delayedCopyBuffer = (uint32_t*)loaded->source;
				delayedCopySize = count;
			}
			loaded = loaded->next;
		}
		if (delayedCopyBuffer != nullptr)
		{
			std::copy(delayedCopyBuffer, delayedCopyBuffer + delayedCopySize, (uint32_t*)(((unsigned char*)stagingBuffer->mapped) + offset));
			offset += delayedCopySize * sizeof(uint32_t);
		}
	}

	if (appState.Scene.paletteSize > 0)
	{
		auto source = d_8to24table;
		auto target = (float*)(((unsigned char*)stagingBuffer->mapped) + offset);
		for (auto i = 0; i < 256; i++)
		{
			auto entry = *source++;
			*target++ = GammaCorrect((float)(entry & 255) / 255);
			*target++ = GammaCorrect((float)((entry >> 8) & 255) / 255);
			*target++ = GammaCorrect((float)((entry >> 16) & 255) / 255);
			*target++ = (float)((entry >> 24) & 255) / 255;
		}
		offset += appState.Scene.paletteSize;
		source = d_8to24table;
		target = (float*)(((unsigned char*)stagingBuffer->mapped) + offset);
		for (auto i = 0; i < 256; i++)
		{
			auto entry = *source++;
			*target++ = (float)(entry & 255) / 255;
			*target++ = (float)((entry >> 8) & 255) / 255;
			*target++ = (float)((entry >> 16) & 255) / 255;
			*target++ = (float)((entry >> 24) & 255) / 255;
		}
		offset += appState.Scene.paletteSize;
	}
	if (appState.Scene.colormapSize > 0)
	{
		memcpy(((unsigned char*)stagingBuffer->mapped) + offset, host_colormap.data(), appState.Scene.colormapSize);
		offset += appState.Scene.colormapSize;
	}
	for (auto& entry : appState.Scene.surfaceTextures)
	{
		auto loadedTexture = entry.first;
		while (loadedTexture != nullptr)
		{
			memcpy(((unsigned char*)stagingBuffer->mapped) + offset, loadedTexture->source, loadedTexture->size);
			GenerateMipmaps(stagingBuffer, offset, loadedTexture);
			offset += loadedTexture->allocated;
			loadedTexture = loadedTexture->next;
		}
	}
	auto loadedTexture = appState.Scene.textures.first;
	while (loadedTexture != nullptr)
	{
		memcpy(((unsigned char*)stagingBuffer->mapped) + offset, loadedTexture->source, loadedTexture->size);
		offset += loadedTexture->size;
		loadedTexture = loadedTexture->next;
	}
	for (auto i = 0; i <= appState.Scene.alias.last; i++)
	{
		auto& alias = d_lists.alias[i];
		if (alias.colormap != nullptr)
		{
			memcpy(((unsigned char*)stagingBuffer->mapped) + offset, alias.colormap, 16384);
			offset += 16384;
		}
	}
	for (auto i = 0; i <= appState.Scene.aliasAlpha.last; i++)
	{
		auto& alias = d_lists.alias_alpha[i];
		if (alias.colormap != nullptr)
		{
			memcpy(((unsigned char*)stagingBuffer->mapped) + offset, alias.colormap, 16384);
			offset += 16384;
		}
	}
	for (auto i = 0; i <= appState.Scene.aliasHoley.last; i++)
	{
		auto& alias = d_lists.alias_holey[i];
		if (alias.colormap != nullptr)
		{
			memcpy(((unsigned char*)stagingBuffer->mapped) + offset, alias.colormap, 16384);
			offset += 16384;
		}
	}
	for (auto i = 0; i <= appState.Scene.aliasHoleyAlpha.last; i++)
	{
		auto& alias = d_lists.alias_holey_alpha[i];
		if (alias.colormap != nullptr)
		{
			memcpy(((unsigned char*)stagingBuffer->mapped) + offset, alias.colormap, 16384);
			offset += 16384;
		}
	}
	for (auto i = 0; i <= appState.Scene.viewmodels.last; i++)
	{
		auto& viewmodel = d_lists.viewmodels[i];
		if (viewmodel.colormap != nullptr)
		{
			memcpy(((unsigned char*)stagingBuffer->mapped) + offset, viewmodel.colormap, 16384);
			offset += 16384;
		}
	}
	for (auto i = 0; i <= appState.Scene.viewmodelsHoley.last; i++)
	{
		auto& viewmodel = d_lists.viewmodels_holey[i];
		if (viewmodel.colormap != nullptr)
		{
			memcpy(((unsigned char*)stagingBuffer->mapped) + offset, viewmodel.colormap, 16384);
			offset += 16384;
		}
	}
	if (appState.Scene.lastSky >= 0)
	{
		auto source = appState.Scene.loadedSky.data;
		auto target = ((unsigned char*)stagingBuffer->mapped) + offset;
		auto width = appState.Scene.loadedSky.width;
		auto doubleWidth = width * 2;
		auto height = appState.Scene.loadedSky.height;
		for (auto i = 0; i < height; i++)
		{
			memcpy(target, source, width);
			source += doubleWidth;
			target += width;
		}
		offset += appState.Scene.loadedSky.size;
	}
	while (offset % 4 != 0)
	{
		offset++;
	}
	if (appState.Scene.lastSkyRGBA >= 0)
	{
		memcpy(((unsigned char*)stagingBuffer->mapped) + offset, appState.Scene.loadedSkyRGBA.data, appState.Scene.loadedSkyRGBA.size);
		offset += appState.Scene.loadedSkyRGBA.size;
	}
	for (auto& entry : appState.Scene.surfaceRGBATextures)
	{
		auto loadedTexture = entry.first;
		while (loadedTexture != nullptr)
		{
			memcpy(((unsigned char*)stagingBuffer->mapped) + offset, loadedTexture->source, loadedTexture->size);
			offset += loadedTexture->allocated;
			loadedTexture = loadedTexture->next;
		}
	}
}

void PerFrame::LoadNonStagedResources(AppState &appState)
{
	auto mapped = (float*)matrices->mapped;
	memcpy(mapped, appState.ViewMatrices.data(), 2 * sizeof(XrMatrix4x4f));
	mapped += 2 * 16;
	memcpy(mapped, appState.ProjectionMatrices.data(), 2 * sizeof(XrMatrix4x4f));
	mapped += 2 * 16;
	memcpy(mapped, &appState.VertexTransform, sizeof(XrMatrix4x4f));
	if ((matrices->properties & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) == 0)
	{
		VkMappedMemoryRange range { VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE };
		range.memory = matrices->memory;
		range.size = VK_WHOLE_SIZE;
		CHECK_VKCMD(vkFlushMappedMemoryRanges(appState.Device, 1, &range));
	}

	if (appState.Scene.sortedVerticesSize > 0)
	{
		sortedVertices = cachedSortedVertices.GetHostVisibleVertexBuffer(appState, appState.Scene.sortedVerticesSize);
		if (sortedVertices->mapped == nullptr)
		{
			CHECK_VKCMD(vkMapMemory(appState.Device, sortedVertices->memory, 0, VK_WHOLE_SIZE, 0, &sortedVertices->mapped));
		}
		uint32_t attributeIndex = 0;
		VkDeviceSize offset = 0;
		SortedSurfaces::LoadVertices(appState.Scene.surfaces.sorted, appState.Scene.surfaces.loaded, attributeIndex, sortedVertices, offset);
		SortedSurfaces::LoadVertices(appState.Scene.surfacesColoredLights.sorted, appState.Scene.surfacesColoredLights.loaded, attributeIndex, sortedVertices, offset);
		SortedSurfaces::LoadVertices(appState.Scene.surfacesRGBA.sorted, appState.Scene.surfacesRGBA.loaded, attributeIndex, sortedVertices, offset);
		SortedSurfaces::LoadVertices(appState.Scene.surfacesRGBAColoredLights.sorted, appState.Scene.surfacesRGBAColoredLights.loaded, attributeIndex, sortedVertices, offset);
		SortedSurfaces::LoadVertices(appState.Scene.surfacesRGBANoGlow.sorted, appState.Scene.surfacesRGBANoGlow.loaded, attributeIndex, sortedVertices, offset);
		SortedSurfaces::LoadVertices(appState.Scene.surfacesRGBANoGlowColoredLights.sorted, appState.Scene.surfacesRGBANoGlowColoredLights.loaded, attributeIndex, sortedVertices, offset);
		SortedSurfaces::LoadVertices(appState.Scene.surfacesRotated.sorted, appState.Scene.surfacesRotated.loaded, attributeIndex, sortedVertices, offset);
		SortedSurfaces::LoadVertices(appState.Scene.surfacesRotatedColoredLights.sorted, appState.Scene.surfacesRotatedColoredLights.loaded, attributeIndex, sortedVertices, offset);
		SortedSurfaces::LoadVertices(appState.Scene.surfacesRotatedRGBA.sorted, appState.Scene.surfacesRotatedRGBA.loaded, attributeIndex, sortedVertices, offset);
		SortedSurfaces::LoadVertices(appState.Scene.surfacesRotatedRGBAColoredLights.sorted, appState.Scene.surfacesRotatedRGBAColoredLights.loaded, attributeIndex, sortedVertices, offset);
		SortedSurfaces::LoadVertices(appState.Scene.surfacesRotatedRGBANoGlow.sorted, appState.Scene.surfacesRotatedRGBANoGlow.loaded, attributeIndex, sortedVertices, offset);
		SortedSurfaces::LoadVertices(appState.Scene.surfacesRotatedRGBANoGlowColoredLights.sorted, appState.Scene.surfacesRotatedRGBANoGlowColoredLights.loaded, attributeIndex, sortedVertices, offset);
		SortedSurfaces::LoadVertices(appState.Scene.fences.sorted, appState.Scene.fences.loaded, attributeIndex, sortedVertices, offset);
		SortedSurfaces::LoadVertices(appState.Scene.fencesColoredLights.sorted, appState.Scene.fencesColoredLights.loaded, attributeIndex, sortedVertices, offset);
		SortedSurfaces::LoadVertices(appState.Scene.fencesRGBA.sorted, appState.Scene.fencesRGBA.loaded, attributeIndex, sortedVertices, offset);
		SortedSurfaces::LoadVertices(appState.Scene.fencesRGBAColoredLights.sorted, appState.Scene.fencesRGBAColoredLights.loaded, attributeIndex, sortedVertices, offset);
		SortedSurfaces::LoadVertices(appState.Scene.fencesRGBANoGlow.sorted, appState.Scene.fencesRGBANoGlow.loaded, attributeIndex, sortedVertices, offset);
		SortedSurfaces::LoadVertices(appState.Scene.fencesRGBANoGlowColoredLights.sorted, appState.Scene.fencesRGBANoGlowColoredLights.loaded, attributeIndex, sortedVertices, offset);
		SortedSurfaces::LoadVertices(appState.Scene.fencesRotated.sorted, appState.Scene.fencesRotated.loaded, attributeIndex, sortedVertices, offset);
		SortedSurfaces::LoadVertices(appState.Scene.fencesRotatedColoredLights.sorted, appState.Scene.fencesRotatedColoredLights.loaded, attributeIndex, sortedVertices, offset);
		SortedSurfaces::LoadVertices(appState.Scene.fencesRotatedRGBA.sorted, appState.Scene.fencesRotatedRGBA.loaded, attributeIndex, sortedVertices, offset);
		SortedSurfaces::LoadVertices(appState.Scene.fencesRotatedRGBAColoredLights.sorted, appState.Scene.fencesRotatedRGBAColoredLights.loaded, attributeIndex, sortedVertices, offset);
		SortedSurfaces::LoadVertices(appState.Scene.fencesRotatedRGBANoGlow.sorted, appState.Scene.fencesRotatedRGBANoGlow.loaded, attributeIndex, sortedVertices, offset);
		SortedSurfaces::LoadVertices(appState.Scene.fencesRotatedRGBANoGlowColoredLights.sorted, appState.Scene.fencesRotatedRGBANoGlowColoredLights.loaded, attributeIndex, sortedVertices, offset);
		SortedSurfaces::LoadVertices(appState.Scene.turbulent.sorted, appState.Scene.turbulent.loaded, attributeIndex, sortedVertices, offset);
		SortedSurfaces::LoadVertices(appState.Scene.turbulentRGBA.sorted, appState.Scene.turbulentRGBA.loaded, attributeIndex, sortedVertices, offset);
		SortedSurfaces::LoadVertices(appState.Scene.turbulentLit.sorted, appState.Scene.turbulentLit.loaded, attributeIndex, sortedVertices, offset);
		SortedSurfaces::LoadVertices(appState.Scene.turbulentColoredLights.sorted, appState.Scene.turbulentColoredLights.loaded, attributeIndex, sortedVertices, offset);
		SortedSurfaces::LoadVertices(appState.Scene.turbulentRGBALit.sorted, appState.Scene.turbulentRGBALit.loaded, attributeIndex, sortedVertices, offset);
		SortedSurfaces::LoadVertices(appState.Scene.turbulentRGBAColoredLights.sorted, appState.Scene.turbulentRGBAColoredLights.loaded, attributeIndex, sortedVertices, offset);
		SortedSurfaces::LoadVertices(appState.Scene.turbulentRotated.sorted, appState.Scene.turbulentRotated.loaded, attributeIndex, sortedVertices, offset);
		SortedSurfaces::LoadVertices(appState.Scene.turbulentRotatedRGBA.sorted, appState.Scene.turbulentRotatedRGBA.loaded, attributeIndex, sortedVertices, offset);
		SortedSurfaces::LoadVertices(appState.Scene.turbulentRotatedLit.sorted, appState.Scene.turbulentRotatedLit.loaded, attributeIndex, sortedVertices, offset);
		SortedSurfaces::LoadVertices(appState.Scene.turbulentRotatedColoredLights.sorted, appState.Scene.turbulentRotatedColoredLights.loaded, attributeIndex, sortedVertices, offset);
		SortedSurfaces::LoadVertices(appState.Scene.turbulentRotatedRGBALit.sorted, appState.Scene.turbulentRotatedRGBALit.loaded, attributeIndex, sortedVertices, offset);
		SortedSurfaces::LoadVertices(appState.Scene.turbulentRotatedRGBAColoredLights.sorted, appState.Scene.turbulentRotatedRGBAColoredLights.loaded, attributeIndex, sortedVertices, offset);
		SortedSurfaces::LoadVertices(appState.Scene.sprites.sorted, appState.Scene.sprites.loaded, sortedVertices, offset);
		if (d_lists.last_particle >= 0)
		{
			auto count = (d_lists.last_particle + 1);
			std::copy(d_lists.particles.data(), d_lists.particles.data() + count, (float*)(((unsigned char*)sortedVertices->mapped) + offset));
		}
		if ((sortedVertices->properties & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) == 0)
		{
			VkMappedMemoryRange range { VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE };
			range.memory = sortedVertices->memory;
			range.size = VK_WHOLE_SIZE;
			CHECK_VKCMD(vkFlushMappedMemoryRanges(appState.Device, 1, &range));
		}
	}

	if (appState.Scene.sortedAttributesSize > 0)
	{
		sortedAttributes = cachedSortedAttributes.GetHostVisibleStorageBuffer(appState, appState.Scene.sortedAttributesSize);
		if (sortedAttributes->mapped == nullptr)
		{
			CHECK_VKCMD(vkMapMemory(appState.Device, sortedAttributes->memory, 0, VK_WHOLE_SIZE, 0, &sortedAttributes->mapped));
		}
		VkDeviceSize offset = 0;
		offset = SortedSurfaces::LoadAttributes(appState.Scene.surfaces.sorted, appState.Scene.surfaces.loaded, sortedAttributes, offset);
		offset = SortedSurfaces::LoadAttributes(appState.Scene.surfacesColoredLights.sorted, appState.Scene.surfacesColoredLights.loaded, sortedAttributes, offset);
		offset = SortedSurfaces::LoadAttributes(appState.Scene.surfacesRGBA.sorted, appState.Scene.surfacesRGBA.loaded, sortedAttributes, offset);
		offset = SortedSurfaces::LoadAttributes(appState.Scene.surfacesRGBAColoredLights.sorted, appState.Scene.surfacesRGBAColoredLights.loaded, sortedAttributes, offset);
		offset = SortedSurfaces::LoadAttributes(appState.Scene.surfacesRGBANoGlow.sorted, appState.Scene.surfacesRGBANoGlow.loaded, sortedAttributes, offset);
		offset = SortedSurfaces::LoadAttributes(appState.Scene.surfacesRGBANoGlowColoredLights.sorted, appState.Scene.surfacesRGBANoGlowColoredLights.loaded, sortedAttributes, offset);
		offset = SortedSurfaces::LoadAttributes(appState.Scene.surfacesRotated.sorted, appState.Scene.surfacesRotated.loaded, sortedAttributes, offset);
		offset = SortedSurfaces::LoadAttributes(appState.Scene.surfacesRotatedColoredLights.sorted, appState.Scene.surfacesRotatedColoredLights.loaded, sortedAttributes, offset);
		offset = SortedSurfaces::LoadAttributes(appState.Scene.surfacesRotatedRGBA.sorted, appState.Scene.surfacesRotatedRGBA.loaded, sortedAttributes, offset);
		offset = SortedSurfaces::LoadAttributes(appState.Scene.surfacesRotatedRGBAColoredLights.sorted, appState.Scene.surfacesRotatedRGBAColoredLights.loaded, sortedAttributes, offset);
		offset = SortedSurfaces::LoadAttributes(appState.Scene.surfacesRotatedRGBANoGlow.sorted, appState.Scene.surfacesRotatedRGBANoGlow.loaded, sortedAttributes, offset);
		offset = SortedSurfaces::LoadAttributes(appState.Scene.surfacesRotatedRGBANoGlowColoredLights.sorted, appState.Scene.surfacesRotatedRGBANoGlowColoredLights.loaded, sortedAttributes, offset);
		offset = SortedSurfaces::LoadAttributes(appState.Scene.fences.sorted, appState.Scene.fences.loaded, sortedAttributes, offset);
		offset = SortedSurfaces::LoadAttributes(appState.Scene.fencesColoredLights.sorted, appState.Scene.fencesColoredLights.loaded, sortedAttributes, offset);
		offset = SortedSurfaces::LoadAttributes(appState.Scene.fencesRGBA.sorted, appState.Scene.fencesRGBA.loaded, sortedAttributes, offset);
		offset = SortedSurfaces::LoadAttributes(appState.Scene.fencesRGBAColoredLights.sorted, appState.Scene.fencesRGBAColoredLights.loaded, sortedAttributes, offset);
		offset = SortedSurfaces::LoadAttributes(appState.Scene.fencesRGBANoGlow.sorted, appState.Scene.fencesRGBANoGlow.loaded, sortedAttributes, offset);
		offset = SortedSurfaces::LoadAttributes(appState.Scene.fencesRGBANoGlowColoredLights.sorted, appState.Scene.fencesRGBANoGlowColoredLights.loaded, sortedAttributes, offset);
		offset = SortedSurfaces::LoadAttributes(appState.Scene.fencesRotated.sorted, appState.Scene.fencesRotated.loaded, sortedAttributes, offset);
		offset = SortedSurfaces::LoadAttributes(appState.Scene.fencesRotatedColoredLights.sorted, appState.Scene.fencesRotatedColoredLights.loaded, sortedAttributes, offset);
		offset = SortedSurfaces::LoadAttributes(appState.Scene.fencesRotatedRGBA.sorted, appState.Scene.fencesRotatedRGBA.loaded, sortedAttributes, offset);
		offset = SortedSurfaces::LoadAttributes(appState.Scene.fencesRotatedRGBAColoredLights.sorted, appState.Scene.fencesRotatedRGBAColoredLights.loaded, sortedAttributes, offset);
		offset = SortedSurfaces::LoadAttributes(appState.Scene.fencesRotatedRGBANoGlow.sorted, appState.Scene.fencesRotatedRGBANoGlow.loaded, sortedAttributes, offset);
		offset = SortedSurfaces::LoadAttributes(appState.Scene.fencesRotatedRGBANoGlowColoredLights.sorted, appState.Scene.fencesRotatedRGBANoGlowColoredLights.loaded, sortedAttributes, offset);
		offset = SortedSurfaces::LoadAttributes(appState.Scene.turbulent.sorted, appState.Scene.turbulent.loaded, sortedAttributes, offset);
		offset = SortedSurfaces::LoadAttributes(appState.Scene.turbulentRGBA.sorted, appState.Scene.turbulentRGBA.loaded, sortedAttributes, offset);
		offset = SortedSurfaces::LoadAttributes(appState.Scene.turbulentLit.sorted, appState.Scene.turbulentLit.loaded, sortedAttributes, offset);
		offset = SortedSurfaces::LoadAttributes(appState.Scene.turbulentColoredLights.sorted, appState.Scene.turbulentColoredLights.loaded, sortedAttributes, offset);
		offset = SortedSurfaces::LoadAttributes(appState.Scene.turbulentRGBALit.sorted, appState.Scene.turbulentRGBALit.loaded, sortedAttributes, offset);
		offset = SortedSurfaces::LoadAttributes(appState.Scene.turbulentRGBAColoredLights.sorted, appState.Scene.turbulentRGBAColoredLights.loaded, sortedAttributes, offset);
		offset = SortedSurfaces::LoadAttributes(appState.Scene.turbulentRotated.sorted, appState.Scene.turbulentRotated.loaded, sortedAttributes, offset);
		offset = SortedSurfaces::LoadAttributes(appState.Scene.turbulentRotatedRGBA.sorted, appState.Scene.turbulentRotatedRGBA.loaded, sortedAttributes, offset);
		offset = SortedSurfaces::LoadAttributes(appState.Scene.turbulentRotatedLit.sorted, appState.Scene.turbulentRotatedLit.loaded, sortedAttributes, offset);
		offset = SortedSurfaces::LoadAttributes(appState.Scene.turbulentRotatedColoredLights.sorted, appState.Scene.turbulentRotatedColoredLights.loaded, sortedAttributes, offset);
		offset = SortedSurfaces::LoadAttributes(appState.Scene.turbulentRotatedRGBALit.sorted, appState.Scene.turbulentRotatedRGBALit.loaded, sortedAttributes, offset);
		offset = SortedSurfaces::LoadAttributes(appState.Scene.turbulentRotatedRGBAColoredLights.sorted, appState.Scene.turbulentRotatedRGBAColoredLights.loaded, sortedAttributes, offset);
		if ((sortedAttributes->properties & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) == 0)
		{
			VkMappedMemoryRange range { VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE };
			range.memory = sortedAttributes->memory;
			range.size = VK_WHOLE_SIZE;
			CHECK_VKCMD(vkFlushMappedMemoryRanges(appState.Device, 1, &range));
		}
	}

	if (appState.Scene.sortedIndicesCount > 0)
	{
		if (appState.Scene.sortedIndices16Size > 0)
		{
			sortedIndices16 = cachedSortedIndices16.GetHostVisibleIndexBuffer(appState, appState.Scene.sortedIndices16Size);
			if (sortedIndices16->mapped == nullptr)
			{
				CHECK_VKCMD(vkMapMemory(appState.Device, sortedIndices16->memory, 0, VK_WHOLE_SIZE, 0, &sortedIndices16->mapped));
			}
			VkDeviceSize offset = 0;
			offset = SortedSurfaces::LoadIndices16(appState.Scene.surfaces.sorted, appState.Scene.surfaces.loaded, sortedIndices16, offset);
			offset = SortedSurfaces::LoadIndices16(appState.Scene.surfacesColoredLights.sorted, appState.Scene.surfacesColoredLights.loaded, sortedIndices16, offset);
			offset = SortedSurfaces::LoadIndices16(appState.Scene.surfacesRGBA.sorted, appState.Scene.surfacesRGBA.loaded, sortedIndices16, offset);
			offset = SortedSurfaces::LoadIndices16(appState.Scene.surfacesRGBAColoredLights.sorted, appState.Scene.surfacesRGBAColoredLights.loaded, sortedIndices16, offset);
			offset = SortedSurfaces::LoadIndices16(appState.Scene.surfacesRGBANoGlow.sorted, appState.Scene.surfacesRGBANoGlow.loaded, sortedIndices16, offset);
			offset = SortedSurfaces::LoadIndices16(appState.Scene.surfacesRGBANoGlowColoredLights.sorted, appState.Scene.surfacesRGBANoGlowColoredLights.loaded, sortedIndices16, offset);
			offset = SortedSurfaces::LoadIndices16(appState.Scene.surfacesRotated.sorted, appState.Scene.surfacesRotated.loaded, sortedIndices16, offset);
			offset = SortedSurfaces::LoadIndices16(appState.Scene.surfacesRotatedColoredLights.sorted, appState.Scene.surfacesRotatedColoredLights.loaded, sortedIndices16, offset);
			offset = SortedSurfaces::LoadIndices16(appState.Scene.surfacesRotatedRGBA.sorted, appState.Scene.surfacesRotatedRGBA.loaded, sortedIndices16, offset);
			offset = SortedSurfaces::LoadIndices16(appState.Scene.surfacesRotatedRGBAColoredLights.sorted, appState.Scene.surfacesRotatedRGBAColoredLights.loaded, sortedIndices16, offset);
			offset = SortedSurfaces::LoadIndices16(appState.Scene.surfacesRotatedRGBANoGlow.sorted, appState.Scene.surfacesRotatedRGBANoGlow.loaded, sortedIndices16, offset);
			offset = SortedSurfaces::LoadIndices16(appState.Scene.surfacesRotatedRGBANoGlowColoredLights.sorted, appState.Scene.surfacesRotatedRGBANoGlowColoredLights.loaded, sortedIndices16, offset);
			offset = SortedSurfaces::LoadIndices16(appState.Scene.fences.sorted, appState.Scene.fences.loaded, sortedIndices16, offset);
			offset = SortedSurfaces::LoadIndices16(appState.Scene.fencesColoredLights.sorted, appState.Scene.fencesColoredLights.loaded, sortedIndices16, offset);
			offset = SortedSurfaces::LoadIndices16(appState.Scene.fencesRGBA.sorted, appState.Scene.fencesRGBA.loaded, sortedIndices16, offset);
			offset = SortedSurfaces::LoadIndices16(appState.Scene.fencesRGBAColoredLights.sorted, appState.Scene.fencesRGBAColoredLights.loaded, sortedIndices16, offset);
			offset = SortedSurfaces::LoadIndices16(appState.Scene.fencesRGBANoGlow.sorted, appState.Scene.fencesRGBANoGlow.loaded, sortedIndices16, offset);
			offset = SortedSurfaces::LoadIndices16(appState.Scene.fencesRGBANoGlowColoredLights.sorted, appState.Scene.fencesRGBANoGlowColoredLights.loaded, sortedIndices16, offset);
			offset = SortedSurfaces::LoadIndices16(appState.Scene.fencesRotated.sorted, appState.Scene.fencesRotated.loaded, sortedIndices16, offset);
			offset = SortedSurfaces::LoadIndices16(appState.Scene.fencesRotatedColoredLights.sorted, appState.Scene.fencesRotatedColoredLights.loaded, sortedIndices16, offset);
			offset = SortedSurfaces::LoadIndices16(appState.Scene.fencesRotatedRGBA.sorted, appState.Scene.fencesRotatedRGBA.loaded, sortedIndices16, offset);
			offset = SortedSurfaces::LoadIndices16(appState.Scene.fencesRotatedRGBAColoredLights.sorted, appState.Scene.fencesRotatedRGBAColoredLights.loaded, sortedIndices16, offset);
			offset = SortedSurfaces::LoadIndices16(appState.Scene.fencesRotatedRGBANoGlow.sorted, appState.Scene.fencesRotatedRGBANoGlow.loaded, sortedIndices16, offset);
			offset = SortedSurfaces::LoadIndices16(appState.Scene.fencesRotatedRGBANoGlowColoredLights.sorted, appState.Scene.fencesRotatedRGBANoGlowColoredLights.loaded, sortedIndices16, offset);
			offset = SortedSurfaces::LoadIndices16(appState.Scene.turbulent.sorted, appState.Scene.turbulent.loaded, sortedIndices16, offset);
			offset = SortedSurfaces::LoadIndices16(appState.Scene.turbulentRGBA.sorted, appState.Scene.turbulentRGBA.loaded, sortedIndices16, offset);
			offset = SortedSurfaces::LoadIndices16(appState.Scene.turbulentLit.sorted, appState.Scene.turbulentLit.loaded, sortedIndices16, offset);
			offset = SortedSurfaces::LoadIndices16(appState.Scene.turbulentColoredLights.sorted, appState.Scene.turbulentColoredLights.loaded, sortedIndices16, offset);
			offset = SortedSurfaces::LoadIndices16(appState.Scene.turbulentRGBALit.sorted, appState.Scene.turbulentRGBALit.loaded, sortedIndices16, offset);
			offset = SortedSurfaces::LoadIndices16(appState.Scene.turbulentRGBAColoredLights.sorted, appState.Scene.turbulentRGBAColoredLights.loaded, sortedIndices16, offset);
			offset = SortedSurfaces::LoadIndices16(appState.Scene.turbulentRotated.sorted, appState.Scene.turbulentRotated.loaded, sortedIndices16, offset);
			offset = SortedSurfaces::LoadIndices16(appState.Scene.turbulentRotatedRGBA.sorted, appState.Scene.turbulentRotatedRGBA.loaded, sortedIndices16, offset);
			offset = SortedSurfaces::LoadIndices16(appState.Scene.turbulentRotatedLit.sorted, appState.Scene.turbulentRotatedLit.loaded, sortedIndices16, offset);
			offset = SortedSurfaces::LoadIndices16(appState.Scene.turbulentRotatedColoredLights.sorted, appState.Scene.turbulentRotatedColoredLights.loaded, sortedIndices16, offset);
			offset = SortedSurfaces::LoadIndices16(appState.Scene.turbulentRotatedRGBALit.sorted, appState.Scene.turbulentRotatedRGBALit.loaded, sortedIndices16, offset);
			offset = SortedSurfaces::LoadIndices16(appState.Scene.turbulentRotatedRGBAColoredLights.sorted, appState.Scene.turbulentRotatedRGBAColoredLights.loaded, sortedIndices16, offset);
			offset = SortedSurfaces::LoadIndices16(appState.Scene.sprites.sorted, appState.Scene.sprites.loaded, sortedIndices16, offset);
			if ((sortedIndices16->properties & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) == 0)
			{
				VkMappedMemoryRange range { VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE };
				range.memory = sortedIndices16->memory;
				range.size = VK_WHOLE_SIZE;
				CHECK_VKCMD(vkFlushMappedMemoryRanges(appState.Device, 1, &range));
			}
		}
		else
		{
			sortedIndices32 = cachedSortedIndices32.GetHostVisibleIndexBuffer(appState, appState.Scene.sortedIndices32Size);
			if (sortedIndices32->mapped == nullptr)
			{
				CHECK_VKCMD(vkMapMemory(appState.Device, sortedIndices32->memory, 0, VK_WHOLE_SIZE, 0, &sortedIndices32->mapped));
			}
			VkDeviceSize offset = 0;
			offset = SortedSurfaces::LoadIndices32(appState.Scene.surfaces.sorted, appState.Scene.surfaces.loaded, sortedIndices32, offset);
			offset = SortedSurfaces::LoadIndices32(appState.Scene.surfacesColoredLights.sorted, appState.Scene.surfacesColoredLights.loaded, sortedIndices32, offset);
			offset = SortedSurfaces::LoadIndices32(appState.Scene.surfacesRGBA.sorted, appState.Scene.surfacesRGBA.loaded, sortedIndices32, offset);
			offset = SortedSurfaces::LoadIndices32(appState.Scene.surfacesRGBAColoredLights.sorted, appState.Scene.surfacesRGBAColoredLights.loaded, sortedIndices32, offset);
			offset = SortedSurfaces::LoadIndices32(appState.Scene.surfacesRGBANoGlow.sorted, appState.Scene.surfacesRGBANoGlow.loaded, sortedIndices32, offset);
			offset = SortedSurfaces::LoadIndices32(appState.Scene.surfacesRGBANoGlowColoredLights.sorted, appState.Scene.surfacesRGBANoGlowColoredLights.loaded, sortedIndices32, offset);
			offset = SortedSurfaces::LoadIndices32(appState.Scene.surfacesRotated.sorted, appState.Scene.surfacesRotated.loaded, sortedIndices32, offset);
			offset = SortedSurfaces::LoadIndices32(appState.Scene.surfacesRotatedColoredLights.sorted, appState.Scene.surfacesRotatedColoredLights.loaded, sortedIndices32, offset);
			offset = SortedSurfaces::LoadIndices32(appState.Scene.surfacesRotatedRGBA.sorted, appState.Scene.surfacesRotatedRGBA.loaded, sortedIndices32, offset);
			offset = SortedSurfaces::LoadIndices32(appState.Scene.surfacesRotatedRGBAColoredLights.sorted, appState.Scene.surfacesRotatedRGBAColoredLights.loaded, sortedIndices32, offset);
			offset = SortedSurfaces::LoadIndices32(appState.Scene.surfacesRotatedRGBANoGlow.sorted, appState.Scene.surfacesRotatedRGBANoGlow.loaded, sortedIndices32, offset);
			offset = SortedSurfaces::LoadIndices32(appState.Scene.surfacesRotatedRGBANoGlowColoredLights.sorted, appState.Scene.surfacesRotatedRGBANoGlowColoredLights.loaded, sortedIndices32, offset);
			offset = SortedSurfaces::LoadIndices32(appState.Scene.fences.sorted, appState.Scene.fences.loaded, sortedIndices32, offset);
			offset = SortedSurfaces::LoadIndices32(appState.Scene.fencesColoredLights.sorted, appState.Scene.fencesColoredLights.loaded, sortedIndices32, offset);
			offset = SortedSurfaces::LoadIndices32(appState.Scene.fencesRGBA.sorted, appState.Scene.fencesRGBA.loaded, sortedIndices32, offset);
			offset = SortedSurfaces::LoadIndices32(appState.Scene.fencesRGBAColoredLights.sorted, appState.Scene.fencesRGBAColoredLights.loaded, sortedIndices32, offset);
			offset = SortedSurfaces::LoadIndices32(appState.Scene.fencesRGBANoGlow.sorted, appState.Scene.fencesRGBANoGlow.loaded, sortedIndices32, offset);
			offset = SortedSurfaces::LoadIndices32(appState.Scene.fencesRGBANoGlowColoredLights.sorted, appState.Scene.fencesRGBANoGlowColoredLights.loaded, sortedIndices32, offset);
			offset = SortedSurfaces::LoadIndices32(appState.Scene.fencesRotated.sorted, appState.Scene.fencesRotated.loaded, sortedIndices32, offset);
			offset = SortedSurfaces::LoadIndices32(appState.Scene.fencesRotatedColoredLights.sorted, appState.Scene.fencesRotatedColoredLights.loaded, sortedIndices32, offset);
			offset = SortedSurfaces::LoadIndices32(appState.Scene.fencesRotatedRGBA.sorted, appState.Scene.fencesRotatedRGBA.loaded, sortedIndices32, offset);
			offset = SortedSurfaces::LoadIndices32(appState.Scene.fencesRotatedRGBAColoredLights.sorted, appState.Scene.fencesRotatedRGBAColoredLights.loaded, sortedIndices32, offset);
			offset = SortedSurfaces::LoadIndices32(appState.Scene.fencesRotatedRGBANoGlow.sorted, appState.Scene.fencesRotatedRGBANoGlow.loaded, sortedIndices32, offset);
			offset = SortedSurfaces::LoadIndices32(appState.Scene.fencesRotatedRGBANoGlowColoredLights.sorted, appState.Scene.fencesRotatedRGBANoGlowColoredLights.loaded, sortedIndices32, offset);
			offset = SortedSurfaces::LoadIndices32(appState.Scene.turbulent.sorted, appState.Scene.turbulent.loaded, sortedIndices32, offset);
			offset = SortedSurfaces::LoadIndices32(appState.Scene.turbulentRGBA.sorted, appState.Scene.turbulentRGBA.loaded, sortedIndices32, offset);
			offset = SortedSurfaces::LoadIndices32(appState.Scene.turbulentLit.sorted, appState.Scene.turbulentLit.loaded, sortedIndices32, offset);
			offset = SortedSurfaces::LoadIndices32(appState.Scene.turbulentColoredLights.sorted, appState.Scene.turbulentColoredLights.loaded, sortedIndices32, offset);
			offset = SortedSurfaces::LoadIndices32(appState.Scene.turbulentRGBALit.sorted, appState.Scene.turbulentRGBALit.loaded, sortedIndices32, offset);
			offset = SortedSurfaces::LoadIndices32(appState.Scene.turbulentRGBAColoredLights.sorted, appState.Scene.turbulentRGBAColoredLights.loaded, sortedIndices32, offset);
			offset = SortedSurfaces::LoadIndices32(appState.Scene.turbulentRotated.sorted, appState.Scene.turbulentRotated.loaded, sortedIndices32, offset);
			offset = SortedSurfaces::LoadIndices32(appState.Scene.turbulentRotatedRGBA.sorted, appState.Scene.turbulentRotatedRGBA.loaded, sortedIndices32, offset);
			offset = SortedSurfaces::LoadIndices32(appState.Scene.turbulentRotatedLit.sorted, appState.Scene.turbulentRotatedLit.loaded, sortedIndices32, offset);
			offset = SortedSurfaces::LoadIndices32(appState.Scene.turbulentRotatedColoredLights.sorted, appState.Scene.turbulentRotatedColoredLights.loaded, sortedIndices32, offset);
			offset = SortedSurfaces::LoadIndices32(appState.Scene.turbulentRotatedRGBALit.sorted, appState.Scene.turbulentRotatedRGBALit.loaded, sortedIndices32, offset);
			offset = SortedSurfaces::LoadIndices32(appState.Scene.turbulentRotatedRGBAColoredLights.sorted, appState.Scene.turbulentRotatedRGBAColoredLights.loaded, sortedIndices32, offset);
			offset = SortedSurfaces::LoadIndices32(appState.Scene.sprites.sorted, appState.Scene.sprites.loaded, sortedIndices32, offset);
			if ((sortedIndices32->properties & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) == 0)
			{
				VkMappedMemoryRange range { VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE };
				range.memory = sortedIndices32->memory;
				range.size = VK_WHOLE_SIZE;
				CHECK_VKCMD(vkFlushMappedMemoryRanges(appState.Device, 1, &range));
			}
		}
	}
}

void PerFrame::FillColormapTextures(AppState& appState, LoadedAlias& loaded)
{
	if (loaded.isHostColormap)
	{
		return;
	}
	Texture* texture;
	if (!colormaps.oldTextures.empty())
	{
		texture = colormaps.oldTextures.front();
		colormaps.oldTextures.pop_front();
	}
	else
	{
		texture = new Texture { };
		texture->Create(appState, 256, 64, VK_FORMAT_R8_UINT, 1, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
	}
	texture->Fill(appState, appState.Scene.stagingBuffer);
	appState.Scene.stagingBuffer.offset += 16384;
	colormaps.MoveToFront(texture);
	loaded.colormap.texture = texture;
	colormapCount++;
}

void PerFrame::FillAliasFromStagingBuffer(AppState& appState, Buffer* stagingBuffer, LoadedIndexBuffer* first, VkBufferCopy& bufferCopy, SharedMemoryBuffer*& previousBuffer) const
{
	auto loaded = first;
	auto delayed = false;
	while (loaded != nullptr)
	{
		if (delayed)
		{
			if (previousBuffer == loaded->indices.buffer && bufferCopy.dstOffset + bufferCopy.size == loaded->indices.offset)
			{
				bufferCopy.size += loaded->size;
			}
			else
			{
				vkCmdCopyBuffer(commandBuffer, stagingBuffer->buffer, previousBuffer->buffer, 1, &bufferCopy);
				bufferCopy.srcOffset += bufferCopy.size;
				delayed = false;
			}
		}
		else
		{
			bufferCopy.dstOffset = loaded->indices.offset;
			bufferCopy.size = loaded->size;
			delayed = true;
		}
		if (previousBuffer != loaded->indices.buffer)
		{
			appState.Scene.AddToVertexInputBarriers(loaded->indices.buffer->buffer, VK_ACCESS_INDEX_READ_BIT);
			previousBuffer = loaded->indices.buffer;
		}
		loaded = loaded->next;
	}
	if (delayed)
	{
		vkCmdCopyBuffer(commandBuffer, stagingBuffer->buffer, previousBuffer->buffer, 1, &bufferCopy);
		bufferCopy.srcOffset += bufferCopy.size;
	}
}

void PerFrame::FillFromStagingBuffer(AppState& appState, Buffer* stagingBuffer, uint32_t swapchainImageIndex)
{
#if !defined(NDEBUG) || defined(ENABLE_DEBUG_UTILS)
	VkDebugUtilsLabelEXT fillLabel { VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT };
	fillLabel.pLabelName = "Fill from staging buffer";
	appState.vkCmdBeginDebugUtilsLabelEXT(commandBuffer, &fillLabel);
#endif

	appState.Scene.stagingBuffer.lastStartBarrier = -1;
	appState.Scene.stagingBuffer.lastEndBarrier = -1;
    appState.Scene.stagingBuffer.lastVertexShaderEndBarrier = -1;
	appState.Scene.stagingBuffer.descriptorSetsInUse.clear();

	VkBufferCopy bufferCopy { };

	SharedMemoryBuffer* previousBuffer = nullptr;
	FillAliasFromStagingBuffer(appState, stagingBuffer, appState.Scene.indexBuffers.firstAliasIndices8, bufferCopy, previousBuffer);

	while (bufferCopy.srcOffset % 2 != 0)
	{
		bufferCopy.srcOffset++;
	}

	FillAliasFromStagingBuffer(appState, stagingBuffer, appState.Scene.indexBuffers.firstAliasIndices16, bufferCopy, previousBuffer);

	while (bufferCopy.srcOffset % 4 != 0)
	{
		bufferCopy.srcOffset++;
	}

	FillAliasFromStagingBuffer(appState, stagingBuffer, appState.Scene.indexBuffers.firstAliasIndices32, bufferCopy, previousBuffer);

	bufferCopy.dstOffset = 0;

    auto loaded = appState.Scene.aliasBuffers.firstAliasVertices;
    while (loaded != nullptr)
    {
        bufferCopy.size = loaded->size;
        vkCmdCopyBuffer(commandBuffer, stagingBuffer->buffer, loaded->buffer->buffer, 1, &bufferCopy);
        bufferCopy.srcOffset += loaded->size;
        appState.Scene.AddToVertexInputBarriers(loaded->buffer->buffer, VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT);
        loaded = loaded->next;
    }

	auto loadedTexCoordsBuffer = appState.Scene.aliasBuffers.firstAliasTexCoords;
	while (loadedTexCoordsBuffer != nullptr)
	{
		bufferCopy.size = loadedTexCoordsBuffer->size;
		vkCmdCopyBuffer(commandBuffer, stagingBuffer->buffer, loadedTexCoordsBuffer->buffer->buffer, 1, &bufferCopy);
		bufferCopy.srcOffset += bufferCopy.size;
		appState.Scene.AddToVertexInputBarriers(loadedTexCoordsBuffer->buffer->buffer, VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT);
		loadedTexCoordsBuffer = loadedTexCoordsBuffer->next;
	}

	if (appState.Scene.verticesSize > 0)
	{
		bufferCopy.size = appState.Scene.verticesSize;
		vkCmdCopyBuffer(commandBuffer, stagingBuffer->buffer, vertices->buffer, 1, &bufferCopy);
		bufferCopy.srcOffset += bufferCopy.size;
		appState.Scene.AddToVertexInputBarriers(vertices->buffer, VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT);
	}
	if (appState.Scene.attributesSize > 0)
	{
		bufferCopy.size = appState.Scene.attributesSize;
		vkCmdCopyBuffer(commandBuffer, stagingBuffer->buffer, attributes->buffer, 1, &bufferCopy);
		bufferCopy.srcOffset += bufferCopy.size;
		appState.Scene.AddToVertexInputBarriers(attributes->buffer, VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT);
	}
	if (appState.Scene.colorsSize > 0)
	{
		bufferCopy.size = appState.Scene.colorsSize;
		vkCmdCopyBuffer(commandBuffer, stagingBuffer->buffer, colors->buffer, 1, &bufferCopy);
		bufferCopy.srcOffset += bufferCopy.size;
		appState.Scene.AddToVertexInputBarriers(colors->buffer, VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT);
	}

	if (appState.Scene.indices8Size > 0)
	{
		bufferCopy.size = appState.Scene.indices8Size;
		vkCmdCopyBuffer(commandBuffer, stagingBuffer->buffer, indices8->buffer, 1, &bufferCopy);
		bufferCopy.srcOffset += bufferCopy.size;
		appState.Scene.AddToVertexInputBarriers(indices8->buffer, VK_ACCESS_INDEX_READ_BIT);

		while (bufferCopy.srcOffset % 4 != 0)
		{
			bufferCopy.srcOffset++;
		}
	}
	if (appState.Scene.indices16Size > 0)
	{
		bufferCopy.size = appState.Scene.indices16Size;
		vkCmdCopyBuffer(commandBuffer, stagingBuffer->buffer, indices16->buffer, 1, &bufferCopy);
		bufferCopy.srcOffset += bufferCopy.size;
		appState.Scene.AddToVertexInputBarriers(indices16->buffer, VK_ACCESS_INDEX_READ_BIT);

		while (bufferCopy.srcOffset % 4 != 0)
		{
			bufferCopy.srcOffset++;
		}
	}
	if (appState.Scene.indices32Size > 0)
	{
		bufferCopy.size = appState.Scene.indices32Size;
		vkCmdCopyBuffer(commandBuffer, stagingBuffer->buffer, indices32->buffer, 1, &bufferCopy);
		bufferCopy.srcOffset += bufferCopy.size;
		appState.Scene.AddToVertexInputBarriers(indices32->buffer, VK_ACCESS_INDEX_READ_BIT);
	}

#if !defined(NDEBUG) || defined(ENABLE_DEBUG_UTILS)
	if (!appState.Scene.lightmapChains.empty()) {
	fillLabel.pLabelName = "Lightmaps";
	appState.vkCmdBeginDebugUtilsLabelEXT(commandBuffer, &fillLabel);
#endif
	for (auto& chain : appState.Scene.lightmapChains)
	{
		std::unordered_set<LightmapBuffer*> inUse;
		VkBuffer delayedCopyBuffer = VK_NULL_HANDLE;
		VkDeviceSize delayedCopyOffset = 0;
		VkDeviceSize delayedCopySize = 0;
		auto loaded = chain.first;
		while (loaded != nullptr)
		{
			auto buffer = loaded->lightmap->buffer;
			if (inUse.insert(buffer).second)
			{
				appState.Scene.AddToVertexShaderBarriers(buffer->buffer.buffer, VK_ACCESS_SHADER_READ_BIT);
			}
			if (delayedCopyBuffer == VK_NULL_HANDLE)
			{
				delayedCopyBuffer = buffer->buffer.buffer;
				delayedCopyOffset = loaded->lightmap->offset * sizeof(uint32_t);
				delayedCopySize = loaded->size;
			}
			else if (delayedCopyBuffer == buffer->buffer.buffer && delayedCopyOffset + delayedCopySize == loaded->lightmap->offset * sizeof(uint32_t))
			{
				delayedCopySize += loaded->size;
			}
			else
			{
				bufferCopy.size = delayedCopySize;
				bufferCopy.dstOffset = delayedCopyOffset;
				vkCmdCopyBuffer(commandBuffer, stagingBuffer->buffer, delayedCopyBuffer, 1, &bufferCopy);
				bufferCopy.dstOffset = 0;
				bufferCopy.srcOffset += bufferCopy.size;
				appState.Scene.stagingBuffer.offset += bufferCopy.size;
				delayedCopyBuffer = buffer->buffer.buffer;
				delayedCopyOffset = loaded->lightmap->offset * sizeof(uint32_t);
				delayedCopySize = loaded->size;
			}
			loaded = loaded->next;
		}
		if (delayedCopyBuffer != VK_NULL_HANDLE)
		{
			bufferCopy.size = delayedCopySize;
			bufferCopy.dstOffset = delayedCopyOffset;
			vkCmdCopyBuffer(commandBuffer, stagingBuffer->buffer, delayedCopyBuffer, 1, &bufferCopy);
			bufferCopy.dstOffset = 0;
			bufferCopy.srcOffset += bufferCopy.size;
			appState.Scene.stagingBuffer.offset += bufferCopy.size;
		}
	}
#if !defined(NDEBUG) || defined(ENABLE_DEBUG_UTILS)
	appState.vkCmdEndDebugUtilsLabelEXT(commandBuffer);
	}
#endif

#if !defined(NDEBUG) || defined(ENABLE_DEBUG_UTILS)
	if (!appState.Scene.lightmapRGBChains.empty()) {
	fillLabel.pLabelName = "Lightmaps (RGB)";
	appState.vkCmdBeginDebugUtilsLabelEXT(commandBuffer, &fillLabel);
#endif
	for (auto& chain : appState.Scene.lightmapRGBChains)
	{
		std::unordered_set<LightmapRGBBuffer*> inUse;
		VkBuffer delayedCopyBuffer = VK_NULL_HANDLE;
		VkDeviceSize delayedCopyOffset = 0;
		VkDeviceSize delayedCopySize = 0;
		auto loaded = chain.first;
		while (loaded != nullptr)
		{
			auto buffer = loaded->lightmap->buffer;
			if (inUse.insert(buffer).second)
			{
				appState.Scene.AddToVertexShaderBarriers(buffer->buffer.buffer, VK_ACCESS_SHADER_READ_BIT);
			}
			if (delayedCopyBuffer == VK_NULL_HANDLE)
			{
				delayedCopyBuffer = buffer->buffer.buffer;
				delayedCopyOffset = loaded->lightmap->offset * sizeof(uint32_t);
				delayedCopySize = loaded->size;
			}
			else if (delayedCopyBuffer == buffer->buffer.buffer && delayedCopyOffset + delayedCopySize == loaded->lightmap->offset * sizeof(uint32_t))
			{
				delayedCopySize += loaded->size;
			}
			else
			{
				bufferCopy.size = delayedCopySize;
				bufferCopy.dstOffset = delayedCopyOffset;
				vkCmdCopyBuffer(commandBuffer, stagingBuffer->buffer, delayedCopyBuffer, 1, &bufferCopy);
				bufferCopy.dstOffset = 0;
				bufferCopy.srcOffset += bufferCopy.size;
				appState.Scene.stagingBuffer.offset += bufferCopy.size;
				delayedCopyBuffer = buffer->buffer.buffer;
				delayedCopyOffset = loaded->lightmap->offset * sizeof(uint32_t);
				delayedCopySize = loaded->size;
			}
			loaded = loaded->next;
		}
		if (delayedCopyBuffer != VK_NULL_HANDLE)
		{
			bufferCopy.size = delayedCopySize;
			bufferCopy.dstOffset = delayedCopyOffset;
			vkCmdCopyBuffer(commandBuffer, stagingBuffer->buffer, delayedCopyBuffer, 1, &bufferCopy);
			bufferCopy.dstOffset = 0;
			bufferCopy.srcOffset += bufferCopy.size;
			appState.Scene.stagingBuffer.offset += bufferCopy.size;
		}
	}
#if !defined(NDEBUG) || defined(ENABLE_DEBUG_UTILS)
	appState.vkCmdEndDebugUtilsLabelEXT(commandBuffer);
	}
#endif

	if (appState.Scene.stagingBuffer.lastEndBarrier >= 0)
	{
		vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, 0, 0, nullptr, appState.Scene.stagingBuffer.lastEndBarrier + 1, appState.Scene.stagingBuffer.vertexInputEndBarriers.data(), 0, nullptr);
	}

    if (appState.Scene.stagingBuffer.lastVertexShaderEndBarrier >= 0)
    {
        vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_VERTEX_SHADER_BIT, 0, 0, nullptr, appState.Scene.stagingBuffer.lastVertexShaderEndBarrier + 1, appState.Scene.stagingBuffer.vertexShaderEndBarriers.data(), 0, nullptr);
    }

    if (appState.Scene.paletteSize > 0)
	{
		bufferCopy.size = appState.Scene.paletteSize;
		vkCmdCopyBuffer(commandBuffer, stagingBuffer->buffer, appState.Scene.paletteBuffers[swapchainImageIndex], 1, &bufferCopy);
		bufferCopy.srcOffset += bufferCopy.size;

		VkBufferMemoryBarrier barrier { };
		barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
		barrier.buffer = appState.Scene.paletteBuffers[swapchainImageIndex];
		barrier.size = VK_WHOLE_SIZE;
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
		vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 1, &barrier, 0, nullptr);

		vkCmdCopyBuffer(commandBuffer, stagingBuffer->buffer, appState.Scene.neutralPaletteBuffers[swapchainImageIndex], 1, &bufferCopy);
		bufferCopy.srcOffset += bufferCopy.size;

		barrier.buffer = appState.Scene.paletteBuffers[swapchainImageIndex];
		vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 1, &barrier, 0, nullptr);
	}

	appState.Scene.stagingBuffer.buffer = stagingBuffer;
	appState.Scene.stagingBuffer.offset = bufferCopy.srcOffset;
	appState.Scene.stagingBuffer.commandBuffer = commandBuffer;
	appState.Scene.stagingBuffer.lastEndBarrier = -1;

	if (appState.Scene.colormapSize > 0)
	{
        appState.Scene.colormap.Fill(appState, appState.Scene.stagingBuffer);
		appState.Scene.stagingBuffer.offset += appState.Scene.colormapSize;
	}

	if (appState.Scene.stagingBuffer.lastStartBarrier >= 0)
	{
		vkCmdPipelineBarrier(appState.Scene.stagingBuffer.commandBuffer, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, appState.Scene.stagingBuffer.lastStartBarrier + 1, appState.Scene.stagingBuffer.imageStartBarriers.data());
	}

	for (auto& entry : appState.Scene.surfaceTextures)
	{
		auto loadedTexture = entry.first;
		while (loadedTexture != nullptr)
		{
			loadedTexture->texture->FillMipmapped(appState, appState.Scene.stagingBuffer, loadedTexture->index);
			appState.Scene.stagingBuffer.offset += loadedTexture->allocated;
			loadedTexture = loadedTexture->next;
		}
	}
	auto loadedTexture = appState.Scene.textures.first;
	while (loadedTexture != nullptr)
	{
		loadedTexture->texture->FillMipmapped(appState, appState.Scene.stagingBuffer, loadedTexture->mips, loadedTexture->index);
		appState.Scene.stagingBuffer.offset += loadedTexture->size;
		loadedTexture = loadedTexture->next;
	}

	for (auto i = 0; i <= appState.Scene.alias.last; i++)
	{
		FillColormapTextures(appState, appState.Scene.alias.loaded[i]);
	}
	for (auto i = 0; i <= appState.Scene.aliasAlpha.last; i++)
	{
		FillColormapTextures(appState, appState.Scene.aliasAlpha.loaded[i]);
	}
	for (auto i = 0; i <= appState.Scene.aliasHoley.last; i++)
	{
		FillColormapTextures(appState, appState.Scene.aliasHoley.loaded[i]);
	}
	for (auto i = 0; i <= appState.Scene.aliasHoleyAlpha.last; i++)
	{
		FillColormapTextures(appState, appState.Scene.aliasHoleyAlpha.loaded[i]);
	}
	for (auto i = 0; i <= appState.Scene.viewmodels.last; i++)
	{
		FillColormapTextures(appState, appState.Scene.viewmodels.loaded[i]);
	}
	for (auto i = 0; i <= appState.Scene.viewmodelsHoley.last; i++)
	{
		FillColormapTextures(appState, appState.Scene.viewmodelsHoley.loaded[i]);
	}

	if (appState.Scene.lastSky >= 0)
	{
		sky->Fill(appState, appState.Scene.stagingBuffer);
		appState.Scene.stagingBuffer.offset += appState.Scene.loadedSky.size;
	}

	while (appState.Scene.stagingBuffer.offset % 4 != 0)
	{
		appState.Scene.stagingBuffer.offset++;
	}

	if (appState.Scene.lastSkyRGBA >= 0)
	{
		skyRGBA->Fill(appState, appState.Scene.stagingBuffer);
		appState.Scene.stagingBuffer.offset += appState.Scene.loadedSkyRGBA.size;
	}

	for (auto& entry : appState.Scene.surfaceRGBATextures)
	{
		auto loadedTexture = entry.first;
		while (loadedTexture != nullptr)
		{
			loadedTexture->texture->FillMipmapped(appState, appState.Scene.stagingBuffer, loadedTexture->mips, loadedTexture->index);
			appState.Scene.stagingBuffer.offset += loadedTexture->allocated;
			loadedTexture = loadedTexture->next;
		}
	}

	if (appState.Scene.stagingBuffer.lastEndBarrier >= 0)
	{
		vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, appState.Scene.stagingBuffer.lastEndBarrier + 1, appState.Scene.stagingBuffer.imageEndBarriers.data());
	}

#if !defined(NDEBUG) || defined(ENABLE_DEBUG_UTILS)
	appState.vkCmdEndDebugUtilsLabelEXT(commandBuffer);
#endif
}

void PerFrame::Reset(AppState& appState)
{
	colors = nullptr;
	sortedIndices32 = nullptr;
    indices32 = nullptr;
	sortedIndices16 = nullptr;
    indices16 = nullptr;
    indices8 = nullptr;
    sortedAttributes = nullptr;
    attributes = nullptr;
	sortedVertices = nullptr;
    vertices = nullptr;
    colormapCount = 0;
    colormaps.Reset(appState);
    stagingBuffers.Reset(appState);
    cachedColors.Reset(appState);
	cachedSortedIndices32.Reset(appState);
    cachedIndices32.Reset(appState);
	cachedSortedIndices16.Reset(appState);
    cachedIndices16.Reset(appState);
    cachedIndices8.Reset(appState);
    cachedSortedAttributes.Reset(appState);
    cachedAttributes.Reset(appState);
	cachedSortedVertices.Reset(appState);
    cachedVertices.Reset(appState);
}

void PerFrame::SetPushConstants(const LoadedAliasColoredLights& loaded, float pushConstants[])
{
	pushConstants[0] = loaded.transform[0][0];
	pushConstants[1] = loaded.transform[1][0];
	pushConstants[2] = loaded.transform[2][0];
	pushConstants[4] = loaded.transform[0][1];
	pushConstants[5] = loaded.transform[1][1];
	pushConstants[6] = loaded.transform[2][1];
	pushConstants[8] = loaded.transform[0][2];
	pushConstants[9] = loaded.transform[1][2];
	pushConstants[10] = loaded.transform[2][2];
	pushConstants[12] = loaded.transform[0][3];
	pushConstants[13] = loaded.transform[1][3];
	pushConstants[14] = loaded.transform[2][3];
}

void PerFrame::SetTintPushConstants(float* pushConstants, size_t offset)
{
	pushConstants[0 + offset] = v_blend[0] * 255;
	pushConstants[1 + offset] = v_blend[1] * 255;
	pushConstants[2 + offset] = v_blend[2] * 255;
	pushConstants[3 + offset] = v_blend[3];
	pushConstants[4 + offset] = v_gamma.value;
}

void PerFrame::Render(AppState& appState, uint32_t swapchainImageIndex)
{
	VkDescriptorPoolSize poolSizes[3] { };

	VkDescriptorPoolCreateInfo descriptorPoolCreateInfo { VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO };
	descriptorPoolCreateInfo.maxSets = 1;
	descriptorPoolCreateInfo.pPoolSizes = poolSizes;

	VkDescriptorSetAllocateInfo descriptorSetAllocateInfo { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
	descriptorSetAllocateInfo.descriptorSetCount = 1;

    VkDescriptorBufferInfo bufferInfo[2] { };

	VkDescriptorImageInfo textureInfo { };
	textureInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

	VkWriteDescriptorSet writes[3] { };
	writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	writes[0].descriptorCount = 1;
	writes[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	writes[1].descriptorCount = 1;
	writes[1].dstBinding = 1;
	writes[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	writes[2].descriptorCount = 1;
	writes[2].dstBinding = 2;

	if (!host_colormapResources.created && appState.Scene.colormap.image != VK_NULL_HANDLE)
	{
		poolSizes[0].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		poolSizes[0].descriptorCount = 1;
		textureInfo.sampler = appState.Scene.sampler;
		textureInfo.imageView = appState.Scene.colormap.view;
		writes[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		writes[0].pImageInfo = &textureInfo;
		descriptorPoolCreateInfo.poolSizeCount = 1;
		CHECK_VKCMD(vkCreateDescriptorPool(appState.Device, &descriptorPoolCreateInfo, nullptr, &host_colormapResources.descriptorPool));
		descriptorSetAllocateInfo.descriptorPool = host_colormapResources.descriptorPool;
		descriptorSetAllocateInfo.pSetLayouts = &appState.Scene.singleImageLayout;
		CHECK_VKCMD(vkAllocateDescriptorSets(appState.Device, &descriptorSetAllocateInfo, &host_colormapResources.descriptorSet));
		writes[0].dstSet = host_colormapResources.descriptorSet;
		vkUpdateDescriptorSets(appState.Device, 1, writes, 0, nullptr);
		host_colormapResources.created = true;
	}
	if (!sceneMatricesResources.created || !sceneMatricesAndPaletteResources.created || !sceneMatricesAndNeutralPaletteResources.created || (!sceneMatricesAndColormapResources.created && appState.Scene.colormap.image != VK_NULL_HANDLE))
	{
		poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		poolSizes[0].descriptorCount = 1;
		bufferInfo[0].buffer = matrices->buffer;
		bufferInfo[0].range = VK_WHOLE_SIZE;
		writes[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		writes[0].pBufferInfo = bufferInfo;
		if (!sceneMatricesResources.created)
		{
			descriptorPoolCreateInfo.poolSizeCount = 1;
			CHECK_VKCMD(vkCreateDescriptorPool(appState.Device, &descriptorPoolCreateInfo, nullptr, &sceneMatricesResources.descriptorPool));
			descriptorSetAllocateInfo.descriptorPool = sceneMatricesResources.descriptorPool;
			descriptorSetAllocateInfo.pSetLayouts = &appState.Scene.singleBufferLayout;
			CHECK_VKCMD(vkAllocateDescriptorSets(appState.Device, &descriptorSetAllocateInfo, &sceneMatricesResources.descriptorSet));
			writes[0].dstSet = sceneMatricesResources.descriptorSet;
			vkUpdateDescriptorSets(appState.Device, 1, writes, 0, nullptr);
			sceneMatricesResources.created = true;
		}
		if (!sceneMatricesAndPaletteResources.created || !sceneMatricesAndNeutralPaletteResources.created || (!sceneMatricesAndColormapResources.created && appState.Scene.colormap.image != VK_NULL_HANDLE))
		{
			poolSizes[1].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			poolSizes[1].descriptorCount = 1;
			writes[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			writes[1].pBufferInfo = bufferInfo + 1;
			if (!sceneMatricesAndPaletteResources.created)
			{
				bufferInfo[1].buffer = appState.Scene.paletteBuffers[swapchainImageIndex];
				bufferInfo[1].range = appState.Scene.paletteBufferSize;
				descriptorPoolCreateInfo.poolSizeCount = 2;
				CHECK_VKCMD(vkCreateDescriptorPool(appState.Device, &descriptorPoolCreateInfo, nullptr, &sceneMatricesAndPaletteResources.descriptorPool));
				descriptorSetAllocateInfo.descriptorPool = sceneMatricesAndPaletteResources.descriptorPool;
				descriptorSetAllocateInfo.pSetLayouts = &appState.Scene.doubleBufferLayout;
				CHECK_VKCMD(vkAllocateDescriptorSets(appState.Device, &descriptorSetAllocateInfo, &sceneMatricesAndPaletteResources.descriptorSet));
				writes[0].dstSet = sceneMatricesAndPaletteResources.descriptorSet;
				writes[1].dstSet = sceneMatricesAndPaletteResources.descriptorSet;
				vkUpdateDescriptorSets(appState.Device, 2, writes, 0, nullptr);
				sceneMatricesAndPaletteResources.created = true;
			}
			if (!sceneMatricesAndNeutralPaletteResources.created)
			{
				bufferInfo[1].buffer = appState.Scene.neutralPaletteBuffers[swapchainImageIndex];
				bufferInfo[1].range = appState.Scene.paletteBufferSize;
				descriptorPoolCreateInfo.poolSizeCount = 2;
				CHECK_VKCMD(vkCreateDescriptorPool(appState.Device, &descriptorPoolCreateInfo, nullptr, &sceneMatricesAndNeutralPaletteResources.descriptorPool));
				descriptorSetAllocateInfo.descriptorPool = sceneMatricesAndNeutralPaletteResources.descriptorPool;
				descriptorSetAllocateInfo.pSetLayouts = &appState.Scene.doubleBufferLayout;
				CHECK_VKCMD(vkAllocateDescriptorSets(appState.Device, &descriptorSetAllocateInfo, &sceneMatricesAndNeutralPaletteResources.descriptorSet));
				writes[0].dstSet = sceneMatricesAndNeutralPaletteResources.descriptorSet;
				writes[1].dstSet = sceneMatricesAndNeutralPaletteResources.descriptorSet;
				vkUpdateDescriptorSets(appState.Device, 2, writes, 0, nullptr);
				sceneMatricesAndNeutralPaletteResources.created = true;
			}
			if (!sceneMatricesAndColormapResources.created && appState.Scene.colormap.image != VK_NULL_HANDLE)
			{
				bufferInfo[1].buffer = appState.Scene.paletteBuffers[swapchainImageIndex];
				bufferInfo[1].range = appState.Scene.paletteBufferSize;
				poolSizes[2].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
				poolSizes[2].descriptorCount = 1;
				textureInfo.sampler = appState.Scene.sampler;
				textureInfo.imageView = appState.Scene.colormap.view;
				writes[2].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
				writes[2].pImageInfo = &textureInfo;
				descriptorPoolCreateInfo.poolSizeCount = 3;
				CHECK_VKCMD(vkCreateDescriptorPool(appState.Device, &descriptorPoolCreateInfo, nullptr, &sceneMatricesAndColormapResources.descriptorPool));
				descriptorSetAllocateInfo.descriptorPool = sceneMatricesAndColormapResources.descriptorPool;
				descriptorSetAllocateInfo.pSetLayouts = &appState.Scene.twoBuffersAndImageLayout;
				CHECK_VKCMD(vkAllocateDescriptorSets(appState.Device, &descriptorSetAllocateInfo, &sceneMatricesAndColormapResources.descriptorSet));
				writes[0].dstSet = sceneMatricesAndColormapResources.descriptorSet;
				writes[1].dstSet = sceneMatricesAndColormapResources.descriptorSet;
				writes[2].dstSet = sceneMatricesAndColormapResources.descriptorSet;
				vkUpdateDescriptorSets(appState.Device, 3, writes, 0, nullptr);
				sceneMatricesAndColormapResources.created = true;
			}
		}
	}
    if (sortedAttributes != nullptr && previousSortedAttributes != sortedAttributes)
    {
        poolSizes[0].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        poolSizes[0].descriptorCount = 1;
        bufferInfo[0].buffer = sortedAttributes->buffer;
        bufferInfo[0].range = sortedAttributes->size;
        if (!sortedAttributesResources.created)
        {
            descriptorPoolCreateInfo.poolSizeCount = 1;
            CHECK_VKCMD(vkCreateDescriptorPool(appState.Device, &descriptorPoolCreateInfo, nullptr, &sortedAttributesResources.descriptorPool));
            descriptorSetAllocateInfo.descriptorPool = sortedAttributesResources.descriptorPool;
            descriptorSetAllocateInfo.pSetLayouts = &appState.Scene.singleStorageBufferLayout;
            CHECK_VKCMD(vkAllocateDescriptorSets(appState.Device, &descriptorSetAllocateInfo, &sortedAttributesResources.descriptorSet));
            sortedAttributesResources.created = true;
        }
        writes[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        writes[0].pBufferInfo = bufferInfo;
        writes[0].dstSet = sortedAttributesResources.descriptorSet;
        vkUpdateDescriptorSets(appState.Device, 1, writes, 0, nullptr);
        previousSortedAttributes = sortedAttributes;
    }
	if (!appState.Scene.lightmapDescriptorWrites.empty())
	{
		auto size = appState.Scene.lightmapDescriptorWrites.size();
		for (size_t i = 0; i < size; i++)
		{
			appState.Scene.lightmapDescriptorWrites[i].pBufferInfo = &appState.Scene.lightmapDescriptorInfos[i];
		}
		vkUpdateDescriptorSets(appState.Device, (int)size, appState.Scene.lightmapDescriptorWrites.data(), 0, nullptr);
	}
#if !defined(NDEBUG) || defined(ENABLE_DEBUG_UTILS)
	VkDebugUtilsLabelEXT renderLabel { VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT };
	renderLabel.pLabelName = "Render";
	appState.vkCmdBeginDebugUtilsLabelEXT(commandBuffer, &renderLabel);
#endif
	if (appState.Mode == AppWorldMode)
	{
		poolSizes[0].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		writes[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		writes[0].pImageInfo = &textureInfo;
		float pushConstants[21];
		if (appState.Scene.lastSky >= 0)
		{
			poolSizes[0].descriptorCount = 1;
			descriptorPoolCreateInfo.poolSizeCount = 1;
			if (!skyResources.created)
			{
				CHECK_VKCMD(vkCreateDescriptorPool(appState.Device, &descriptorPoolCreateInfo, nullptr, &skyResources.descriptorPool));
				descriptorSetAllocateInfo.descriptorPool = skyResources.descriptorPool;
				descriptorSetAllocateInfo.descriptorSetCount = 1;
				descriptorSetAllocateInfo.pSetLayouts = &appState.Scene.singleImageLayout;
				CHECK_VKCMD(vkAllocateDescriptorSets(appState.Device, &descriptorSetAllocateInfo, &skyResources.descriptorSet));
				textureInfo.sampler = appState.Scene.sampler;
				textureInfo.imageView = sky->view;
				writes[0].dstSet = skyResources.descriptorSet;
				vkUpdateDescriptorSets(appState.Device, 1, writes, 0, nullptr);
				skyResources.created = true;
			}
#if !defined(NDEBUG) || defined(ENABLE_DEBUG_UTILS)
			renderLabel.pLabelName = SKY_NAME;
			appState.vkCmdBeginDebugUtilsLabelEXT(commandBuffer, &renderLabel);
#endif
			vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.sky.pipeline);
			vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertices->buffer, &skyVertexBase);
			vkCmdBindVertexBuffers(commandBuffer, 1, 1, &attributes->buffer, &skyAttributeBase);
			VkDescriptorSet descriptorSets[2];
			descriptorSets[0] = sceneMatricesAndPaletteResources.descriptorSet;
			descriptorSets[1] = skyResources.descriptorSet;
			vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.sky.pipelineLayout, 0, 2, descriptorSets, 0, nullptr);
			XrMatrix4x4f orientation;
			XrMatrix4x4f_CreateFromQuaternion(&orientation, &appState.CameraLocation.pose.orientation);
			pushConstants[0] = -orientation.m[8];
			pushConstants[1] = orientation.m[10];
			pushConstants[2] = -orientation.m[9];
			pushConstants[3] = orientation.m[0];
			pushConstants[4] = -orientation.m[2];
			pushConstants[5] = orientation.m[1];
			pushConstants[6] = orientation.m[4];
			pushConstants[7] = -orientation.m[6];
			pushConstants[8] = orientation.m[5];
			pushConstants[9] = appState.EyeTextureWidth;
			pushConstants[10] = appState.EyeTextureHeight;
			pushConstants[11] = appState.EyeTextureMaxDimension;
			pushConstants[12] = skytime*skyspeed;
			vkCmdPushConstants(commandBuffer, appState.Scene.sky.pipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, 13 * sizeof(float), &pushConstants);
			vkCmdDraw(commandBuffer, appState.Scene.loadedSky.count, 1, appState.Scene.loadedSky.firstVertex, 0);
#if !defined(NDEBUG) || defined(ENABLE_DEBUG_UTILS)
			appState.vkCmdEndDebugUtilsLabelEXT(commandBuffer);
#endif
		}
		if (appState.Scene.lastSkyRGBA >= 0)
		{
			poolSizes[0].descriptorCount = 1;
			descriptorPoolCreateInfo.poolSizeCount = 1;
			if (!skyRGBAResources.created)
			{
				CHECK_VKCMD(vkCreateDescriptorPool(appState.Device, &descriptorPoolCreateInfo, nullptr, &skyRGBAResources.descriptorPool));
				descriptorSetAllocateInfo.descriptorPool = skyRGBAResources.descriptorPool;
				descriptorSetAllocateInfo.descriptorSetCount = 1;
				descriptorSetAllocateInfo.pSetLayouts = &appState.Scene.singleImageLayout;
				CHECK_VKCMD(vkAllocateDescriptorSets(appState.Device, &descriptorSetAllocateInfo, &skyRGBAResources.descriptorSet));
				textureInfo.sampler = appState.Scene.sampler;
				textureInfo.imageView = skyRGBA->view;
				writes[0].dstSet = skyRGBAResources.descriptorSet;
				vkUpdateDescriptorSets(appState.Device, 1, writes, 0, nullptr);
				skyRGBAResources.created = true;
			}
#if !defined(NDEBUG) || defined(ENABLE_DEBUG_UTILS)
			renderLabel.pLabelName = SKY_RGBA_NAME;
			appState.vkCmdBeginDebugUtilsLabelEXT(commandBuffer, &renderLabel);
#endif
			vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.skyRGBA.pipeline);
			vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertices->buffer, &skyVertexBase);
			vkCmdBindVertexBuffers(commandBuffer, 1, 1, &attributes->buffer, &skyAttributeBase);
			VkDescriptorSet descriptorSets[2];
			descriptorSets[0] = sceneMatricesResources.descriptorSet;
			descriptorSets[1] = skyRGBAResources.descriptorSet;
			vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.skyRGBA.pipelineLayout, 0, 2, descriptorSets, 0, nullptr);
			XrMatrix4x4f orientation;
			XrMatrix4x4f_CreateFromQuaternion(&orientation, &appState.CameraLocation.pose.orientation);
			pushConstants[0] = -orientation.m[8];
			pushConstants[1] = orientation.m[10];
			pushConstants[2] = -orientation.m[9];
			pushConstants[3] = orientation.m[0];
			pushConstants[4] = -orientation.m[2];
			pushConstants[5] = orientation.m[1];
			pushConstants[6] = orientation.m[4];
			pushConstants[7] = -orientation.m[6];
			pushConstants[8] = orientation.m[5];
			pushConstants[9] = appState.EyeTextureWidth;
			pushConstants[10] = appState.EyeTextureHeight;
			pushConstants[11] = appState.EyeTextureMaxDimension;
			pushConstants[12] = skytime*skyspeed;
			pushConstants[13] = appState.Scene.loadedSkyRGBA.width;
			pushConstants[14] = appState.Scene.loadedSkyRGBA.height;
			vkCmdPushConstants(commandBuffer, appState.Scene.skyRGBA.pipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, 15 * sizeof(float), &pushConstants);
			vkCmdDraw(commandBuffer, appState.Scene.loadedSkyRGBA.count, 1, appState.Scene.loadedSkyRGBA.firstVertex, 0);
#if !defined(NDEBUG) || defined(ENABLE_DEBUG_UTILS)
			appState.vkCmdEndDebugUtilsLabelEXT(commandBuffer);
#endif
		}
		if (appState.Scene.surfaces.last >= 0)
		{
#if !defined(NDEBUG) || defined(ENABLE_DEBUG_UTILS)
			renderLabel.pLabelName = SURFACES_NAME;
			appState.vkCmdBeginDebugUtilsLabelEXT(commandBuffer, &renderLabel);
#endif
			vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.surfaces.pipeline);
			vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.surfaces.pipelineLayout, 0, 1, &sceneMatricesAndColormapResources.descriptorSet, 0, nullptr);
			auto vertexBase = appState.Scene.surfaces.vertexBase;
			vkCmdBindVertexBuffers(commandBuffer, 0, 1, &sortedVertices->buffer, &vertexBase);
            vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.surfaces.pipelineLayout, 1, 1, &sortedAttributesResources.descriptorSet, 0, nullptr);
			if (appState.Scene.sortedIndices16Size > 0)
			{
				vkCmdBindIndexBuffer(commandBuffer, sortedIndices16->buffer, appState.Scene.surfaces.indexBase, VK_INDEX_TYPE_UINT16);
			}
			else
			{
				vkCmdBindIndexBuffer(commandBuffer, sortedIndices32->buffer, appState.Scene.surfaces.indexBase, VK_INDEX_TYPE_UINT32);
			}
			appState.Scene.surfaces.Render(commandBuffer);
#if !defined(NDEBUG) || defined(ENABLE_DEBUG_UTILS)
			appState.vkCmdEndDebugUtilsLabelEXT(commandBuffer);
#endif
		}
		if (appState.Scene.surfacesColoredLights.last >= 0)
		{
#if !defined(NDEBUG) || defined(ENABLE_DEBUG_UTILS)
			renderLabel.pLabelName = SURFACES_COLORED_LIGHTS_NAME;
			appState.vkCmdBeginDebugUtilsLabelEXT(commandBuffer, &renderLabel);
#endif
			vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.surfacesColoredLights.pipeline);
			vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.surfacesColoredLights.pipelineLayout, 0, 1, &sceneMatricesAndNeutralPaletteResources.descriptorSet, 0, nullptr);
			auto vertexBase = appState.Scene.surfacesColoredLights.vertexBase;
			vkCmdBindVertexBuffers(commandBuffer, 0, 1, &sortedVertices->buffer, &vertexBase);
            vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.surfacesColoredLights.pipelineLayout, 1, 1, &sortedAttributesResources.descriptorSet, 0, nullptr);
			if (appState.Scene.sortedIndices16Size > 0)
			{
				vkCmdBindIndexBuffer(commandBuffer, sortedIndices16->buffer, appState.Scene.surfacesColoredLights.indexBase, VK_INDEX_TYPE_UINT16);
			}
			else
			{
				vkCmdBindIndexBuffer(commandBuffer, sortedIndices32->buffer, appState.Scene.surfacesColoredLights.indexBase, VK_INDEX_TYPE_UINT32);
			}
			SetTintPushConstants(pushConstants);
			vkCmdPushConstants(commandBuffer, appState.Scene.surfacesColoredLights.pipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, 5 * sizeof(float), &pushConstants);
			appState.Scene.surfacesColoredLights.Render(commandBuffer);
#if !defined(NDEBUG) || defined(ENABLE_DEBUG_UTILS)
			appState.vkCmdEndDebugUtilsLabelEXT(commandBuffer);
#endif
		}
		if (appState.Scene.surfacesRGBA.last >= 0)
		{
#if !defined(NDEBUG) || defined(ENABLE_DEBUG_UTILS)
			renderLabel.pLabelName = SURFACES_RGBA_NAME;
			appState.vkCmdBeginDebugUtilsLabelEXT(commandBuffer, &renderLabel);
#endif
			vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.surfacesRGBA.pipeline);
			vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.surfacesRGBA.pipelineLayout, 0, 1, &sceneMatricesResources.descriptorSet, 0, nullptr);
			auto vertexBase = appState.Scene.surfacesRGBA.vertexBase;
			vkCmdBindVertexBuffers(commandBuffer, 0, 1, &sortedVertices->buffer, &vertexBase);
            vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.surfacesRGBA.pipelineLayout, 1, 1, &sortedAttributesResources.descriptorSet, 0, nullptr);
			if (appState.Scene.sortedIndices16Size > 0)
			{
				vkCmdBindIndexBuffer(commandBuffer, sortedIndices16->buffer, appState.Scene.surfacesRGBA.indexBase, VK_INDEX_TYPE_UINT16);
			}
			else
			{
				vkCmdBindIndexBuffer(commandBuffer, sortedIndices32->buffer, appState.Scene.surfacesRGBA.indexBase, VK_INDEX_TYPE_UINT32);
			}
			SetTintPushConstants(pushConstants);
			vkCmdPushConstants(commandBuffer, appState.Scene.surfacesRGBA.pipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, 5 * sizeof(float), &pushConstants);
			appState.Scene.surfacesRGBA.Render(commandBuffer);
#if !defined(NDEBUG) || defined(ENABLE_DEBUG_UTILS)
			appState.vkCmdEndDebugUtilsLabelEXT(commandBuffer);
#endif
		}
		if (appState.Scene.surfacesRGBAColoredLights.last >= 0)
		{
#if !defined(NDEBUG) || defined(ENABLE_DEBUG_UTILS)
			renderLabel.pLabelName = SURFACES_RGBA_COLORED_LIGHTS_NAME;
			appState.vkCmdBeginDebugUtilsLabelEXT(commandBuffer, &renderLabel);
#endif
			vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.surfacesRGBAColoredLights.pipeline);
			vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.surfacesRGBAColoredLights.pipelineLayout, 0, 1, &sceneMatricesResources.descriptorSet, 0, nullptr);
			auto vertexBase = appState.Scene.surfacesRGBAColoredLights.vertexBase;
			vkCmdBindVertexBuffers(commandBuffer, 0, 1, &sortedVertices->buffer, &vertexBase);
            vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.surfacesRGBAColoredLights.pipelineLayout, 1, 1, &sortedAttributesResources.descriptorSet, 0, nullptr);
			if (appState.Scene.sortedIndices16Size > 0)
			{
				vkCmdBindIndexBuffer(commandBuffer, sortedIndices16->buffer, appState.Scene.surfacesRGBAColoredLights.indexBase, VK_INDEX_TYPE_UINT16);
			}
			else
			{
				vkCmdBindIndexBuffer(commandBuffer, sortedIndices32->buffer, appState.Scene.surfacesRGBAColoredLights.indexBase, VK_INDEX_TYPE_UINT32);
			}
			SetTintPushConstants(pushConstants);
			vkCmdPushConstants(commandBuffer, appState.Scene.surfacesRGBAColoredLights.pipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, 5 * sizeof(float), &pushConstants);
			appState.Scene.surfacesRGBAColoredLights.Render(commandBuffer);
#if !defined(NDEBUG) || defined(ENABLE_DEBUG_UTILS)
			appState.vkCmdEndDebugUtilsLabelEXT(commandBuffer);
#endif
		}
		if (appState.Scene.surfacesRGBANoGlow.last >= 0)
		{
#if !defined(NDEBUG) || defined(ENABLE_DEBUG_UTILS)
			renderLabel.pLabelName = SURFACES_RGBA_NO_GLOW_NAME;
			appState.vkCmdBeginDebugUtilsLabelEXT(commandBuffer, &renderLabel);
#endif
			vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.surfacesRGBANoGlow.pipeline);
			vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.surfacesRGBANoGlow.pipelineLayout, 0, 1, &sceneMatricesResources.descriptorSet, 0, nullptr);
			auto vertexBase = appState.Scene.surfacesRGBANoGlow.vertexBase;
			vkCmdBindVertexBuffers(commandBuffer, 0, 1, &sortedVertices->buffer, &vertexBase);
            vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.surfacesRGBANoGlow.pipelineLayout, 1, 1, &sortedAttributesResources.descriptorSet, 0, nullptr);
			if (appState.Scene.sortedIndices16Size > 0)
			{
				vkCmdBindIndexBuffer(commandBuffer, sortedIndices16->buffer, appState.Scene.surfacesRGBANoGlow.indexBase, VK_INDEX_TYPE_UINT16);
			}
			else
			{
				vkCmdBindIndexBuffer(commandBuffer, sortedIndices32->buffer, appState.Scene.surfacesRGBANoGlow.indexBase, VK_INDEX_TYPE_UINT32);
			}
			SetTintPushConstants(pushConstants);
			vkCmdPushConstants(commandBuffer, appState.Scene.surfacesRGBANoGlow.pipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, 5 * sizeof(float), pushConstants);
			appState.Scene.surfacesRGBANoGlow.Render(commandBuffer);
#if !defined(NDEBUG) || defined(ENABLE_DEBUG_UTILS)
			appState.vkCmdEndDebugUtilsLabelEXT(commandBuffer);
#endif
		}
		if (appState.Scene.surfacesRGBANoGlowColoredLights.last >= 0)
		{
#if !defined(NDEBUG) || defined(ENABLE_DEBUG_UTILS)
			renderLabel.pLabelName = SURFACES_RGBA_NO_GLOW_COLORED_LIGHTS_NAME;
			appState.vkCmdBeginDebugUtilsLabelEXT(commandBuffer, &renderLabel);
#endif
			vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.surfacesRGBANoGlowColoredLights.pipeline);
			vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.surfacesRGBANoGlowColoredLights.pipelineLayout, 0, 1, &sceneMatricesResources.descriptorSet, 0, nullptr);
			auto vertexBase = appState.Scene.surfacesRGBANoGlowColoredLights.vertexBase;
			vkCmdBindVertexBuffers(commandBuffer, 0, 1, &sortedVertices->buffer, &vertexBase);
            vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.surfacesRGBANoGlowColoredLights.pipelineLayout, 1, 1, &sortedAttributesResources.descriptorSet, 0, nullptr);
			if (appState.Scene.sortedIndices16Size > 0)
			{
				vkCmdBindIndexBuffer(commandBuffer, sortedIndices16->buffer, appState.Scene.surfacesRGBANoGlowColoredLights.indexBase, VK_INDEX_TYPE_UINT16);
			}
			else
			{
				vkCmdBindIndexBuffer(commandBuffer, sortedIndices32->buffer, appState.Scene.surfacesRGBANoGlowColoredLights.indexBase, VK_INDEX_TYPE_UINT32);
			}
			SetTintPushConstants(pushConstants);
			vkCmdPushConstants(commandBuffer, appState.Scene.surfacesRGBANoGlowColoredLights.pipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, 5 * sizeof(float), pushConstants);
			appState.Scene.surfacesRGBANoGlowColoredLights.Render(commandBuffer);
#if !defined(NDEBUG) || defined(ENABLE_DEBUG_UTILS)
			appState.vkCmdEndDebugUtilsLabelEXT(commandBuffer);
#endif
		}
		if (appState.Scene.turbulent.last >= 0)
		{
#if !defined(NDEBUG) || defined(ENABLE_DEBUG_UTILS)
			renderLabel.pLabelName = TURBULENT_NAME;
			appState.vkCmdBeginDebugUtilsLabelEXT(commandBuffer, &renderLabel);
#endif
			vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.turbulent.pipeline);
			vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.turbulent.pipelineLayout, 0, 1, &sceneMatricesAndPaletteResources.descriptorSet, 0, nullptr);
			auto vertexBase = appState.Scene.turbulent.vertexBase;
			vkCmdBindVertexBuffers(commandBuffer, 0, 1, &sortedVertices->buffer, &vertexBase);
            vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.turbulent.pipelineLayout, 1, 1, &sortedAttributesResources.descriptorSet, 0, nullptr);
			if (appState.Scene.sortedIndices16Size > 0)
			{
				vkCmdBindIndexBuffer(commandBuffer, sortedIndices16->buffer, appState.Scene.turbulent.indexBase, VK_INDEX_TYPE_UINT16);
			}
			else
			{
				vkCmdBindIndexBuffer(commandBuffer, sortedIndices32->buffer, appState.Scene.turbulent.indexBase, VK_INDEX_TYPE_UINT32);
			}
			auto time = (float)appState.FromEngine.time;
			vkCmdPushConstants(commandBuffer, appState.Scene.turbulent.pipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(float), &time);
			VkDeviceSize indexBase = 0;
			for (auto t = 0; t < appState.Scene.turbulent.sorted.count; t++)
			{
                const auto& texture = appState.Scene.turbulent.sorted.textures[t];
				vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.turbulent.pipelineLayout, 2, 1, &texture.texture, 0, nullptr);
				vkCmdDrawIndexed(commandBuffer, texture.indexCount, 1, indexBase, 0, 0);
				indexBase += texture.indexCount;
			}
#if !defined(NDEBUG) || defined(ENABLE_DEBUG_UTILS)
			appState.vkCmdEndDebugUtilsLabelEXT(commandBuffer);
#endif
		}
		if (appState.Scene.turbulentRGBA.last >= 0)
		{
#if !defined(NDEBUG) || defined(ENABLE_DEBUG_UTILS)
			renderLabel.pLabelName = TURBULENT_RGBA_NAME;
			appState.vkCmdBeginDebugUtilsLabelEXT(commandBuffer, &renderLabel);
#endif
			vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.turbulentRGBA.pipeline);
			vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.turbulentRGBA.pipelineLayout, 0, 1, &sceneMatricesResources.descriptorSet, 0, nullptr);
			auto vertexBase = appState.Scene.turbulentRGBA.vertexBase;
			vkCmdBindVertexBuffers(commandBuffer, 0, 1, &sortedVertices->buffer, &vertexBase);
            vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.turbulentRGBA.pipelineLayout, 1, 1, &sortedAttributesResources.descriptorSet, 0, nullptr);
			if (appState.Scene.sortedIndices16Size > 0)
			{
				vkCmdBindIndexBuffer(commandBuffer, sortedIndices16->buffer, appState.Scene.turbulentRGBA.indexBase, VK_INDEX_TYPE_UINT16);
			}
			else
			{
				vkCmdBindIndexBuffer(commandBuffer, sortedIndices32->buffer, appState.Scene.turbulentRGBA.indexBase, VK_INDEX_TYPE_UINT32);
			}
			SetTintPushConstants(pushConstants);
			pushConstants[5] = (float)appState.FromEngine.time;
			vkCmdPushConstants(commandBuffer, appState.Scene.turbulentRGBA.pipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, 6 * sizeof(float), pushConstants);
			VkDeviceSize indexBase = 0;
            for (auto t = 0; t < appState.Scene.turbulentRGBA.sorted.count; t++)
            {
                const auto& texture = appState.Scene.turbulentRGBA.sorted.textures[t];
				vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.turbulentRGBA.pipelineLayout, 2, 1, &texture.texture, 0, nullptr);
				vkCmdDrawIndexed(commandBuffer, texture.indexCount, 1, indexBase, 0, 0);
				indexBase += texture.indexCount;
			}
#if !defined(NDEBUG) || defined(ENABLE_DEBUG_UTILS)
			appState.vkCmdEndDebugUtilsLabelEXT(commandBuffer);
#endif
		}
		if (appState.Scene.turbulentLit.last >= 0)
		{
#if !defined(NDEBUG) || defined(ENABLE_DEBUG_UTILS)
			renderLabel.pLabelName = TURBULENT_LIT_NAME;
			appState.vkCmdBeginDebugUtilsLabelEXT(commandBuffer, &renderLabel);
#endif
			vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.turbulentLit.pipeline);
			vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.turbulentLit.pipelineLayout, 0, 1, &sceneMatricesAndColormapResources.descriptorSet, 0, nullptr);
			auto vertexBase = appState.Scene.turbulentLit.vertexBase;
			vkCmdBindVertexBuffers(commandBuffer, 0, 1, &sortedVertices->buffer, &vertexBase);
            vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.turbulentLit.pipelineLayout, 1, 1, &sortedAttributesResources.descriptorSet, 0, nullptr);
			if (appState.Scene.sortedIndices16Size > 0)
			{
				vkCmdBindIndexBuffer(commandBuffer, sortedIndices16->buffer, appState.Scene.turbulentLit.indexBase, VK_INDEX_TYPE_UINT16);
			}
			else
			{
				vkCmdBindIndexBuffer(commandBuffer, sortedIndices32->buffer, appState.Scene.turbulentLit.indexBase, VK_INDEX_TYPE_UINT32);
			}
			auto time = (float)appState.FromEngine.time;
			vkCmdPushConstants(commandBuffer, appState.Scene.turbulentLit.pipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(float), &time);
			appState.Scene.turbulentLit.Render(commandBuffer);
#if !defined(NDEBUG) || defined(ENABLE_DEBUG_UTILS)
			appState.vkCmdEndDebugUtilsLabelEXT(commandBuffer);
#endif
		}
		if (appState.Scene.turbulentColoredLights.last >= 0)
		{
#if !defined(NDEBUG) || defined(ENABLE_DEBUG_UTILS)
			renderLabel.pLabelName = TURBULENT_COLORED_LIGHTS_NAME;
			appState.vkCmdBeginDebugUtilsLabelEXT(commandBuffer, &renderLabel);
#endif
			vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.turbulentColoredLights.pipeline);
			vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.turbulentColoredLights.pipelineLayout, 0, 1, &sceneMatricesAndNeutralPaletteResources.descriptorSet, 0, nullptr);
			auto vertexBase = appState.Scene.turbulentColoredLights.vertexBase;
			vkCmdBindVertexBuffers(commandBuffer, 0, 1, &sortedVertices->buffer, &vertexBase);
            vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.turbulentColoredLights.pipelineLayout, 1, 1, &sortedAttributesResources.descriptorSet, 0, nullptr);
			if (appState.Scene.sortedIndices16Size > 0)
			{
				vkCmdBindIndexBuffer(commandBuffer, sortedIndices16->buffer, appState.Scene.turbulentColoredLights.indexBase, VK_INDEX_TYPE_UINT16);
			}
			else
			{
				vkCmdBindIndexBuffer(commandBuffer, sortedIndices32->buffer, appState.Scene.turbulentColoredLights.indexBase, VK_INDEX_TYPE_UINT32);
			}
			SetTintPushConstants(pushConstants);
			pushConstants[5] = (float)appState.FromEngine.time;
			vkCmdPushConstants(commandBuffer, appState.Scene.turbulentColoredLights.pipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, 6 * sizeof(float), pushConstants);
			appState.Scene.turbulentColoredLights.Render(commandBuffer);
#if !defined(NDEBUG) || defined(ENABLE_DEBUG_UTILS)
			appState.vkCmdEndDebugUtilsLabelEXT(commandBuffer);
#endif
		}
		if (appState.Scene.turbulentRGBALit.last >= 0)
		{
#if !defined(NDEBUG) || defined(ENABLE_DEBUG_UTILS)
			renderLabel.pLabelName = TURBULENT_RGBA_LIT_NAME;
			appState.vkCmdBeginDebugUtilsLabelEXT(commandBuffer, &renderLabel);
#endif
			vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.turbulentRGBALit.pipeline);
			vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.turbulentRGBALit.pipelineLayout, 0, 1, &sceneMatricesResources.descriptorSet, 0, nullptr);
			auto vertexBase = appState.Scene.turbulentRGBALit.vertexBase;
			vkCmdBindVertexBuffers(commandBuffer, 0, 1, &sortedVertices->buffer, &vertexBase);
            vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.turbulentRGBALit.pipelineLayout, 1, 1, &sortedAttributesResources.descriptorSet, 0, nullptr);
			if (appState.Scene.sortedIndices16Size > 0)
			{
				vkCmdBindIndexBuffer(commandBuffer, sortedIndices16->buffer, appState.Scene.turbulentRGBALit.indexBase, VK_INDEX_TYPE_UINT16);
			}
			else
			{
				vkCmdBindIndexBuffer(commandBuffer, sortedIndices32->buffer, appState.Scene.turbulentRGBALit.indexBase, VK_INDEX_TYPE_UINT32);
			}
			SetTintPushConstants(pushConstants);
			pushConstants[5] = (float)appState.FromEngine.time;
			vkCmdPushConstants(commandBuffer, appState.Scene.turbulentRGBALit.pipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, 6 * sizeof(float), pushConstants);
			appState.Scene.turbulentRGBALit.Render(commandBuffer);
#if !defined(NDEBUG) || defined(ENABLE_DEBUG_UTILS)
			appState.vkCmdEndDebugUtilsLabelEXT(commandBuffer);
#endif
		}
		if (appState.Scene.turbulentRGBAColoredLights.last >= 0)
		{
#if !defined(NDEBUG) || defined(ENABLE_DEBUG_UTILS)
			renderLabel.pLabelName = TURBULENT_RGBA_COLORED_LIGHTS_NAME;
			appState.vkCmdBeginDebugUtilsLabelEXT(commandBuffer, &renderLabel);
#endif
			vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.turbulentRGBAColoredLights.pipeline);
			vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.turbulentRGBAColoredLights.pipelineLayout, 0, 1, &sceneMatricesResources.descriptorSet, 0, nullptr);
			auto vertexBase = appState.Scene.turbulentRGBAColoredLights.vertexBase;
			vkCmdBindVertexBuffers(commandBuffer, 0, 1, &sortedVertices->buffer, &vertexBase);
            vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.turbulentRGBAColoredLights.pipelineLayout, 1, 1, &sortedAttributesResources.descriptorSet, 0, nullptr);
			if (appState.Scene.sortedIndices16Size > 0)
			{
				vkCmdBindIndexBuffer(commandBuffer, sortedIndices16->buffer, appState.Scene.turbulentRGBAColoredLights.indexBase, VK_INDEX_TYPE_UINT16);
			}
			else
			{
				vkCmdBindIndexBuffer(commandBuffer, sortedIndices32->buffer, appState.Scene.turbulentRGBAColoredLights.indexBase, VK_INDEX_TYPE_UINT32);
			}
			SetTintPushConstants(pushConstants);
			pushConstants[5] = (float)appState.FromEngine.time;
			vkCmdPushConstants(commandBuffer, appState.Scene.turbulentRGBAColoredLights.pipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, 6 * sizeof(float), pushConstants);
			appState.Scene.turbulentRGBAColoredLights.Render(commandBuffer);
#if !defined(NDEBUG) || defined(ENABLE_DEBUG_UTILS)
			appState.vkCmdEndDebugUtilsLabelEXT(commandBuffer);
#endif
		}
		poolSizes[0].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		poolSizes[0].descriptorCount = 1;
		descriptorPoolCreateInfo.poolSizeCount = 1;
		writes[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		writes[0].pImageInfo = &textureInfo;
		if (colormapCount == 0)
		{
			colormapResources.Delete(appState);
		}
		else
		{
			auto size = colormapResources.descriptorSets.size();
			auto required = colormapCount;
			if (size < required || size > required * 2)
			{
				auto toCreate = std::max(4, required + required / 4);
				if (toCreate != size)
				{
					colormapResources.Delete(appState);
					poolSizes[0].descriptorCount = toCreate;
					descriptorPoolCreateInfo.maxSets = toCreate;
					CHECK_VKCMD(vkCreateDescriptorPool(appState.Device, &descriptorPoolCreateInfo, nullptr, &colormapResources.descriptorPool));
					colormapResources.descriptorSetLayouts.resize(toCreate);
					colormapResources.descriptorSets.resize(toCreate);
					colormapResources.bound.resize(toCreate);
					std::fill(colormapResources.descriptorSetLayouts.begin(), colormapResources.descriptorSetLayouts.end(), appState.Scene.singleImageLayout);
					descriptorSetAllocateInfo.descriptorPool = colormapResources.descriptorPool;
					descriptorSetAllocateInfo.descriptorSetCount = toCreate;
					descriptorSetAllocateInfo.pSetLayouts = colormapResources.descriptorSetLayouts.data();
					CHECK_VKCMD(vkAllocateDescriptorSets(appState.Device, &descriptorSetAllocateInfo, colormapResources.descriptorSets.data()));
					colormapResources.created = true;
				}
			}
		}
		auto descriptorSetIndex = 0;
		if (appState.Scene.alias.last >= 0)
		{
#if !defined(NDEBUG) || defined(ENABLE_DEBUG_UTILS)
			renderLabel.pLabelName = ALIAS_NAME;
			appState.vkCmdBeginDebugUtilsLabelEXT(commandBuffer, &renderLabel);
#endif
			vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.alias.pipeline);
			vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.alias.pipelineLayout, 0, 1, &sceneMatricesAndPaletteResources.descriptorSet, 0, nullptr);
			pushConstants[3] = 0;
			pushConstants[7] = 0;
			pushConstants[11] = 0;
			pushConstants[15] = 1;
			SharedMemoryBuffer* previousVertices = nullptr;
			SharedMemoryBuffer* previousTexCoords = nullptr;
			VkDescriptorSet previousColormapDescriptorSet = VK_NULL_HANDLE;
			SharedMemoryTexture* previousTexture = nullptr;
			SharedMemoryBuffer* previousIndices = nullptr;
			for (auto i = 0; i <= appState.Scene.alias.last; i++)
			{
				auto& loaded = appState.Scene.alias.loaded[i];
				auto vertices = loaded.vertices.buffer;
				if (previousVertices != vertices)
				{
					vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertices->buffer, &appState.NoOffset);
					previousVertices = vertices;
				}
				auto texCoords = loaded.texCoords.buffer;
				if (previousTexCoords != texCoords)
				{
					vkCmdBindVertexBuffers(commandBuffer, 1, 1, &texCoords->buffer, &appState.NoOffset);
					previousTexCoords = texCoords;
				}
				VkDeviceSize attributeOffset = aliasAttributeBase + loaded.firstAttribute * sizeof(float);
				vkCmdBindVertexBuffers(commandBuffer, 2, 1, &attributes->buffer, &attributeOffset);
				SetPushConstants(loaded, pushConstants);
				vkCmdPushConstants(commandBuffer, appState.Scene.alias.pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, 16 * sizeof(float), pushConstants);
				if (loaded.isHostColormap)
				{
					if (previousColormapDescriptorSet != host_colormapResources.descriptorSet)
					{
						vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.alias.pipelineLayout, 1, 1, &host_colormapResources.descriptorSet, 0, nullptr);
						previousColormapDescriptorSet = host_colormapResources.descriptorSet;
					}
				}
				else
				{
					auto colormap = loaded.colormap.texture;
					if (colormapResources.bound[descriptorSetIndex] != colormap)
					{
						textureInfo.sampler = appState.Scene.sampler;
						textureInfo.imageView = colormap->view;
						writes[0].dstSet = colormapResources.descriptorSets[descriptorSetIndex];
						vkUpdateDescriptorSets(appState.Device, 1, writes, 0, nullptr);
						colormapResources.bound[descriptorSetIndex] = colormap;
					}
					vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.alias.pipelineLayout, 1, 1, &colormapResources.descriptorSets[descriptorSetIndex], 0, nullptr);
					previousColormapDescriptorSet = colormapResources.descriptorSets[descriptorSetIndex];
					descriptorSetIndex++;
				}
				auto texture = loaded.texture.texture;
				if (previousTexture != texture)
				{
					vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.alias.pipelineLayout, 2, 1, &texture->descriptorSet, 0, nullptr);
					previousTexture = texture;
				}
				auto indices = loaded.indices.indices.buffer;
				if (previousIndices != indices)
				{
					vkCmdBindIndexBuffer(commandBuffer, indices->buffer, 0, loaded.indices.indices.indexType);
					previousIndices = indices;
				}
				vkCmdDrawIndexed(commandBuffer, loaded.count, 1, loaded.indices.indices.firstIndex, 0, 0);
			}
#if !defined(NDEBUG) || defined(ENABLE_DEBUG_UTILS)
			appState.vkCmdEndDebugUtilsLabelEXT(commandBuffer);
#endif
		}
		if (appState.Scene.aliasColoredLights.last >= 0)
		{
#if !defined(NDEBUG) || defined(ENABLE_DEBUG_UTILS)
			renderLabel.pLabelName = ALIAS_COLORED_LIGHTS_NAME;
			appState.vkCmdBeginDebugUtilsLabelEXT(commandBuffer, &renderLabel);
#endif
			vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.aliasColoredLights.pipeline);
			vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.aliasColoredLights.pipelineLayout, 0, 1, &sceneMatricesAndNeutralPaletteResources.descriptorSet, 0, nullptr);
			pushConstants[3] = 0;
			pushConstants[7] = 0;
			pushConstants[11] = 0;
			pushConstants[15] = 1;
			SetTintPushConstants(pushConstants, 16);
			SharedMemoryBuffer* previousVertices = nullptr;
			SharedMemoryBuffer* previousTexCoords = nullptr;
			SharedMemoryTexture* previousTexture = nullptr;
			SharedMemoryBuffer* previousIndices = nullptr;
			for (auto i = 0; i <= appState.Scene.aliasColoredLights.last; i++)
			{
				auto& loaded = appState.Scene.aliasColoredLights.loaded[i];
				auto vertices = loaded.vertices.buffer;
				if (previousVertices != vertices)
				{
					vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertices->buffer, &appState.NoOffset);
					previousVertices = vertices;
				}
				auto texCoords = loaded.texCoords.buffer;
				if (previousTexCoords != texCoords)
				{
					vkCmdBindVertexBuffers(commandBuffer, 1, 1, &texCoords->buffer, &appState.NoOffset);
					previousTexCoords = texCoords;
				}
				VkDeviceSize attributeOffset = aliasAttributeBase + loaded.firstAttribute * sizeof(float);
				vkCmdBindVertexBuffers(commandBuffer, 2, 1, &attributes->buffer, &attributeOffset);
				SetPushConstants(loaded, pushConstants);
				vkCmdPushConstants(commandBuffer, appState.Scene.aliasColoredLights.pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, 21 * sizeof(float), pushConstants);
				auto texture = loaded.texture.texture;
				if (previousTexture != texture)
				{
					vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.aliasColoredLights.pipelineLayout, 1, 1, &texture->descriptorSet, 0, nullptr);
					previousTexture = texture;
				}
				auto indices = loaded.indices.indices.buffer;
				if (previousIndices != indices)
				{
					vkCmdBindIndexBuffer(commandBuffer, indices->buffer, 0, loaded.indices.indices.indexType);
					previousIndices = indices;
				}
				vkCmdDrawIndexed(commandBuffer, loaded.count, 1, loaded.indices.indices.firstIndex, 0, 0);
			}
#if !defined(NDEBUG) || defined(ENABLE_DEBUG_UTILS)
			appState.vkCmdEndDebugUtilsLabelEXT(commandBuffer);
#endif
		}
		if (appState.Scene.lastParticle >= 0)
        {
#if !defined(NDEBUG) || defined(ENABLE_DEBUG_UTILS)
			renderLabel.pLabelName = PARTICLES_NAME;
			appState.vkCmdBeginDebugUtilsLabelEXT(commandBuffer, &renderLabel);
#endif
            vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.particles.pipeline);
			vkCmdBindVertexBuffers(commandBuffer, 0, 1, &sortedVertices->buffer, &particleBase);
            vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.particles.pipelineLayout, 0, 1, &sceneMatricesAndPaletteResources.descriptorSet, 0, nullptr);
            pushConstants[0] = appState.FromEngine.vright0;
            pushConstants[1] = appState.FromEngine.vright1;
            pushConstants[2] = appState.FromEngine.vright2;
            pushConstants[3] = 0;
            pushConstants[4] = appState.FromEngine.vup0;
            pushConstants[5] = appState.FromEngine.vup1;
            pushConstants[6] = appState.FromEngine.vup2;
            pushConstants[7] = 0;
            vkCmdPushConstants(commandBuffer, appState.Scene.particles.pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, 8 * sizeof(float), pushConstants);
            vkCmdDraw(commandBuffer, 6, appState.Scene.lastParticle + 1, 0, 0);
#if !defined(NDEBUG) || defined(ENABLE_DEBUG_UTILS)
			appState.vkCmdEndDebugUtilsLabelEXT(commandBuffer);
#endif
        }
        if (appState.Scene.lastColoredIndex8 >= 0 || appState.Scene.lastColoredIndex16 >= 0 || appState.Scene.lastColoredIndex32 >= 0)
        {
#if !defined(NDEBUG) || defined(ENABLE_DEBUG_UTILS)
			renderLabel.pLabelName = COLORED_NAME;
			appState.vkCmdBeginDebugUtilsLabelEXT(commandBuffer, &renderLabel);
#endif
            vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.colored.pipeline);
            vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertices->buffer, &appState.NoOffset);
            vkCmdBindVertexBuffers(commandBuffer, 1, 1, &colors->buffer, &appState.NoOffset);
            vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.colored.pipelineLayout, 0, 1, &sceneMatricesAndPaletteResources.descriptorSet, 0, nullptr);
            if (appState.IndexTypeUInt8Enabled)
            {
                if (appState.Scene.lastColoredIndex8 >= 0)
                {
                    vkCmdBindIndexBuffer(commandBuffer, indices8->buffer, coloredIndex8Base, VK_INDEX_TYPE_UINT8_EXT);
                    vkCmdDrawIndexed(commandBuffer, appState.Scene.lastColoredIndex8 + 1, 1, 0, 0, 0);
                }
                if (appState.Scene.lastColoredIndex16 >= 0)
                {
                    vkCmdBindIndexBuffer(commandBuffer, indices16->buffer, coloredIndex16Base, VK_INDEX_TYPE_UINT16);
                    vkCmdDrawIndexed(commandBuffer, appState.Scene.lastColoredIndex16 + 1, 1, 0, 0, 0);
                }
            }
            else
            {
                if (appState.Scene.lastColoredIndex8 >= 0 || appState.Scene.lastColoredIndex16 >= 0)
                {
                    vkCmdBindIndexBuffer(commandBuffer, indices16->buffer, coloredIndex16Base, VK_INDEX_TYPE_UINT16);
                    vkCmdDrawIndexed(commandBuffer, appState.Scene.lastColoredIndex8 + 1 + appState.Scene.lastColoredIndex16 + 1, 1, 0, 0, 0);
                }
            }
            if (appState.Scene.lastColoredIndex32 >= 0)
            {
                vkCmdBindIndexBuffer(commandBuffer, indices32->buffer, 0, VK_INDEX_TYPE_UINT32);
                vkCmdDrawIndexed(commandBuffer, appState.Scene.lastColoredIndex32 + 1, 1, 0, 0, 0);
            }
#if !defined(NDEBUG) || defined(ENABLE_DEBUG_UTILS)
			appState.vkCmdEndDebugUtilsLabelEXT(commandBuffer);
#endif
        }
        if (appState.Scene.lastCutoutIndex8 >= 0 || appState.Scene.lastCutoutIndex16 >= 0 || appState.Scene.lastCutoutIndex32 >= 0)
        {
#if !defined(NDEBUG) || defined(ENABLE_DEBUG_UTILS)
			renderLabel.pLabelName = CUTOUT_NAME;
			appState.vkCmdBeginDebugUtilsLabelEXT(commandBuffer, &renderLabel);
#endif
            vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.cutout.pipeline);
            vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertices->buffer, &appState.NoOffset);
            vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.cutout.pipelineLayout, 0, 1, &sceneMatricesAndPaletteResources.descriptorSet, 0, nullptr);
            if (appState.IndexTypeUInt8Enabled)
            {
                if (appState.Scene.lastCutoutIndex8 >= 0)
                {
                    vkCmdBindIndexBuffer(commandBuffer, indices8->buffer, cutoutIndex8Base, VK_INDEX_TYPE_UINT8_EXT);
                    vkCmdDrawIndexed(commandBuffer, appState.Scene.lastCutoutIndex8 + 1, 1, 0, 0, 0);
                }
                if (appState.Scene.lastCutoutIndex16 >= 0)
                {
                    vkCmdBindIndexBuffer(commandBuffer, indices16->buffer, cutoutIndex16Base, VK_INDEX_TYPE_UINT16);
                    vkCmdDrawIndexed(commandBuffer, appState.Scene.lastCutoutIndex16 + 1, 1, 0, 0, 0);
                }
            }
            else
            {
                if (appState.Scene.lastCutoutIndex8 >= 0 || appState.Scene.lastCutoutIndex16 >= 0)
                {
                    vkCmdBindIndexBuffer(commandBuffer, indices16->buffer, cutoutIndex16Base, VK_INDEX_TYPE_UINT16);
                    vkCmdDrawIndexed(commandBuffer, appState.Scene.lastCutoutIndex8 + 1 + appState.Scene.lastCutoutIndex16 + 1, 1, 0, 0, 0);
                }
            }
            if (appState.Scene.lastCutoutIndex32 >= 0)
            {
                vkCmdBindIndexBuffer(commandBuffer, indices32->buffer, cutoutIndex32Base, VK_INDEX_TYPE_UINT32);
                vkCmdDrawIndexed(commandBuffer, appState.Scene.lastCutoutIndex32 + 1, 1, 0, 0, 0);
            }
#if !defined(NDEBUG) || defined(ENABLE_DEBUG_UTILS)
			appState.vkCmdEndDebugUtilsLabelEXT(commandBuffer);
#endif
        }
		if (appState.Scene.surfacesRotated.last >= 0)
		{
#if !defined(NDEBUG) || defined(ENABLE_DEBUG_UTILS)
			renderLabel.pLabelName = SURFACES_ROTATED_NAME;
			appState.vkCmdBeginDebugUtilsLabelEXT(commandBuffer, &renderLabel);
#endif
			vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.surfacesRotated.pipeline);
			vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.surfacesRotated.pipelineLayout, 0, 1, &sceneMatricesAndColormapResources.descriptorSet, 0, nullptr);
			auto vertexBase = appState.Scene.surfacesRotated.vertexBase;
			vkCmdBindVertexBuffers(commandBuffer, 0, 1, &sortedVertices->buffer, &vertexBase);
            vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.surfacesRotated.pipelineLayout, 1, 1, &sortedAttributesResources.descriptorSet, 0, nullptr);
			if (appState.Scene.sortedIndices16Size > 0)
			{
				vkCmdBindIndexBuffer(commandBuffer, sortedIndices16->buffer, appState.Scene.surfacesRotated.indexBase, VK_INDEX_TYPE_UINT16);
			}
			else
			{
				vkCmdBindIndexBuffer(commandBuffer, sortedIndices32->buffer, appState.Scene.surfacesRotated.indexBase, VK_INDEX_TYPE_UINT32);
			}
			appState.Scene.surfacesRotated.Render(commandBuffer);
#if !defined(NDEBUG) || defined(ENABLE_DEBUG_UTILS)
			appState.vkCmdEndDebugUtilsLabelEXT(commandBuffer);
#endif
		}
		if (appState.Scene.surfacesRotatedColoredLights.last >= 0)
		{
#if !defined(NDEBUG) || defined(ENABLE_DEBUG_UTILS)
			renderLabel.pLabelName = SURFACES_ROTATED_COLORED_LIGHTS_NAME;
			appState.vkCmdBeginDebugUtilsLabelEXT(commandBuffer, &renderLabel);
#endif
			vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.surfacesRotatedColoredLights.pipeline);
			vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.surfacesRotatedColoredLights.pipelineLayout, 0, 1, &sceneMatricesAndNeutralPaletteResources.descriptorSet, 0, nullptr);
			auto vertexBase = appState.Scene.surfacesRotatedColoredLights.vertexBase;
			vkCmdBindVertexBuffers(commandBuffer, 0, 1, &sortedVertices->buffer, &vertexBase);
            vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.surfacesRotatedColoredLights.pipelineLayout, 1, 1, &sortedAttributesResources.descriptorSet, 0, nullptr);
			if (appState.Scene.sortedIndices16Size > 0)
			{
				vkCmdBindIndexBuffer(commandBuffer, sortedIndices16->buffer, appState.Scene.surfacesRotatedColoredLights.indexBase, VK_INDEX_TYPE_UINT16);
			}
			else
			{
				vkCmdBindIndexBuffer(commandBuffer, sortedIndices32->buffer, appState.Scene.surfacesRotatedColoredLights.indexBase, VK_INDEX_TYPE_UINT32);
			}
			SetTintPushConstants(pushConstants);
			vkCmdPushConstants(commandBuffer, appState.Scene.surfacesRotatedColoredLights.pipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, 5 * sizeof(float), &pushConstants);
			appState.Scene.surfacesRotatedColoredLights.Render(commandBuffer);
#if !defined(NDEBUG) || defined(ENABLE_DEBUG_UTILS)
			appState.vkCmdEndDebugUtilsLabelEXT(commandBuffer);
#endif
		}
		if (appState.Scene.surfacesRotatedRGBA.last >= 0)
		{
#if !defined(NDEBUG) || defined(ENABLE_DEBUG_UTILS)
			renderLabel.pLabelName = SURFACES_ROTATED_RGBA_NAME;
			appState.vkCmdBeginDebugUtilsLabelEXT(commandBuffer, &renderLabel);
#endif
			vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.surfacesRotatedRGBA.pipeline);
			vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.surfacesRotatedRGBA.pipelineLayout, 0, 1, &sceneMatricesResources.descriptorSet, 0, nullptr);
			auto vertexBase = appState.Scene.surfacesRotatedRGBA.vertexBase;
			vkCmdBindVertexBuffers(commandBuffer, 0, 1, &sortedVertices->buffer, &vertexBase);
            vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.surfacesRotatedRGBA.pipelineLayout, 1, 1, &sortedAttributesResources.descriptorSet, 0, nullptr);
			if (appState.Scene.sortedIndices16Size > 0)
			{
				vkCmdBindIndexBuffer(commandBuffer, sortedIndices16->buffer, appState.Scene.surfacesRotatedRGBA.indexBase, VK_INDEX_TYPE_UINT16);
			}
			else
			{
				vkCmdBindIndexBuffer(commandBuffer, sortedIndices32->buffer, appState.Scene.surfacesRotatedRGBA.indexBase, VK_INDEX_TYPE_UINT32);
			}
			SetTintPushConstants(pushConstants);
			vkCmdPushConstants(commandBuffer, appState.Scene.surfacesRotatedRGBA.pipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, 5 * sizeof(float), &pushConstants);
			appState.Scene.surfacesRotatedRGBA.Render(commandBuffer);
#if !defined(NDEBUG) || defined(ENABLE_DEBUG_UTILS)
			appState.vkCmdEndDebugUtilsLabelEXT(commandBuffer);
#endif
		}
		if (appState.Scene.surfacesRotatedRGBAColoredLights.last >= 0)
		{
#if !defined(NDEBUG) || defined(ENABLE_DEBUG_UTILS)
			renderLabel.pLabelName = SURFACES_ROTATED_RGBA_COLORED_LIGHTS_NAME;
			appState.vkCmdBeginDebugUtilsLabelEXT(commandBuffer, &renderLabel);
#endif
			vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.surfacesRotatedRGBAColoredLights.pipeline);
			vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.surfacesRotatedRGBAColoredLights.pipelineLayout, 0, 1, &sceneMatricesResources.descriptorSet, 0, nullptr);
			auto vertexBase = appState.Scene.surfacesRotatedRGBAColoredLights.vertexBase;
			vkCmdBindVertexBuffers(commandBuffer, 0, 1, &sortedVertices->buffer, &vertexBase);
            vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.surfacesRotatedRGBAColoredLights.pipelineLayout, 1, 1, &sortedAttributesResources.descriptorSet, 0, nullptr);
			if (appState.Scene.sortedIndices16Size > 0)
			{
				vkCmdBindIndexBuffer(commandBuffer, sortedIndices16->buffer, appState.Scene.surfacesRotatedRGBAColoredLights.indexBase, VK_INDEX_TYPE_UINT16);
			}
			else
			{
				vkCmdBindIndexBuffer(commandBuffer, sortedIndices32->buffer, appState.Scene.surfacesRotatedRGBAColoredLights.indexBase, VK_INDEX_TYPE_UINT32);
			}
			SetTintPushConstants(pushConstants);
			vkCmdPushConstants(commandBuffer, appState.Scene.surfacesRotatedRGBAColoredLights.pipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, 5 * sizeof(float), &pushConstants);
			appState.Scene.surfacesRotatedRGBAColoredLights.Render(commandBuffer);
#if !defined(NDEBUG) || defined(ENABLE_DEBUG_UTILS)
			appState.vkCmdEndDebugUtilsLabelEXT(commandBuffer);
#endif
		}
		if (appState.Scene.surfacesRotatedRGBANoGlow.last >= 0)
		{
#if !defined(NDEBUG) || defined(ENABLE_DEBUG_UTILS)
			renderLabel.pLabelName = SURFACES_ROTATED_RGBA_NO_GLOW_NAME;
			appState.vkCmdBeginDebugUtilsLabelEXT(commandBuffer, &renderLabel);
#endif
			vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.surfacesRotatedRGBANoGlow.pipeline);
			vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.surfacesRotatedRGBANoGlow.pipelineLayout, 0, 1, &sceneMatricesResources.descriptorSet, 0, nullptr);
			auto vertexBase = appState.Scene.surfacesRotatedRGBANoGlow.vertexBase;
			vkCmdBindVertexBuffers(commandBuffer, 0, 1, &sortedVertices->buffer, &vertexBase);
            vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.surfacesRotatedRGBANoGlow.pipelineLayout, 1, 1, &sortedAttributesResources.descriptorSet, 0, nullptr);
			if (appState.Scene.sortedIndices16Size > 0)
			{
				vkCmdBindIndexBuffer(commandBuffer, sortedIndices16->buffer, appState.Scene.surfacesRotatedRGBANoGlow.indexBase, VK_INDEX_TYPE_UINT16);
			}
			else
			{
				vkCmdBindIndexBuffer(commandBuffer, sortedIndices32->buffer, appState.Scene.surfacesRotatedRGBANoGlow.indexBase, VK_INDEX_TYPE_UINT32);
			}
			SetTintPushConstants(pushConstants);
			vkCmdPushConstants(commandBuffer, appState.Scene.surfacesRotatedRGBANoGlow.pipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, 5 * sizeof(float), &pushConstants);
			appState.Scene.surfacesRotatedRGBANoGlow.Render(commandBuffer);
#if !defined(NDEBUG) || defined(ENABLE_DEBUG_UTILS)
			appState.vkCmdEndDebugUtilsLabelEXT(commandBuffer);
#endif
		}
		if (appState.Scene.surfacesRotatedRGBANoGlowColoredLights.last >= 0)
		{
#if !defined(NDEBUG) || defined(ENABLE_DEBUG_UTILS)
			renderLabel.pLabelName = SURFACES_ROTATED_RGBA_NO_GLOW_COLORED_LIGHTS_NAME;
			appState.vkCmdBeginDebugUtilsLabelEXT(commandBuffer, &renderLabel);
#endif
			vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.surfacesRotatedRGBANoGlowColoredLights.pipeline);
			vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.surfacesRotatedRGBANoGlowColoredLights.pipelineLayout, 0, 1, &sceneMatricesResources.descriptorSet, 0, nullptr);
			auto vertexBase = appState.Scene.surfacesRotatedRGBANoGlowColoredLights.vertexBase;
			vkCmdBindVertexBuffers(commandBuffer, 0, 1, &sortedVertices->buffer, &vertexBase);
            vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.surfacesRotatedRGBANoGlowColoredLights.pipelineLayout, 1, 1, &sortedAttributesResources.descriptorSet, 0, nullptr);
			if (appState.Scene.sortedIndices16Size > 0)
			{
				vkCmdBindIndexBuffer(commandBuffer, sortedIndices16->buffer, appState.Scene.surfacesRotatedRGBANoGlowColoredLights.indexBase, VK_INDEX_TYPE_UINT16);
			}
			else
			{
				vkCmdBindIndexBuffer(commandBuffer, sortedIndices32->buffer, appState.Scene.surfacesRotatedRGBANoGlowColoredLights.indexBase, VK_INDEX_TYPE_UINT32);
			}
			SetTintPushConstants(pushConstants);
			vkCmdPushConstants(commandBuffer, appState.Scene.surfacesRotatedRGBANoGlowColoredLights.pipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, 5 * sizeof(float), &pushConstants);
			appState.Scene.surfacesRotatedRGBANoGlowColoredLights.Render(commandBuffer);
#if !defined(NDEBUG) || defined(ENABLE_DEBUG_UTILS)
			appState.vkCmdEndDebugUtilsLabelEXT(commandBuffer);
#endif
		}
        if (appState.Scene.fences.last >= 0)
        {
#if !defined(NDEBUG) || defined(ENABLE_DEBUG_UTILS)
			renderLabel.pLabelName = FENCES_NAME;
			appState.vkCmdBeginDebugUtilsLabelEXT(commandBuffer, &renderLabel);
#endif
            vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.fences.pipeline);
            vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.fences.pipelineLayout, 0, 1, &sceneMatricesAndColormapResources.descriptorSet, 0, nullptr);
            auto vertexBase = appState.Scene.fences.vertexBase;
            vkCmdBindVertexBuffers(commandBuffer, 0, 1, &sortedVertices->buffer, &vertexBase);
            vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.fences.pipelineLayout, 1, 1, &sortedAttributesResources.descriptorSet, 0, nullptr);
            if (appState.Scene.sortedIndices16Size > 0)
            {
                vkCmdBindIndexBuffer(commandBuffer, sortedIndices16->buffer, appState.Scene.fences.indexBase, VK_INDEX_TYPE_UINT16);
            }
            else
            {
                vkCmdBindIndexBuffer(commandBuffer, sortedIndices32->buffer, appState.Scene.fences.indexBase, VK_INDEX_TYPE_UINT32);
            }
			appState.Scene.fences.Render(commandBuffer);
#if !defined(NDEBUG) || defined(ENABLE_DEBUG_UTILS)
			appState.vkCmdEndDebugUtilsLabelEXT(commandBuffer);
#endif
        }
        if (appState.Scene.fencesColoredLights.last >= 0)
        {
#if !defined(NDEBUG) || defined(ENABLE_DEBUG_UTILS)
			renderLabel.pLabelName = FENCES_COLORED_LIGHTS_NAME;
			appState.vkCmdBeginDebugUtilsLabelEXT(commandBuffer, &renderLabel);
#endif
            vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.fencesColoredLights.pipeline);
            vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.fencesColoredLights.pipelineLayout, 0, 1, &sceneMatricesAndNeutralPaletteResources.descriptorSet, 0, nullptr);
            auto vertexBase = appState.Scene.fencesColoredLights.vertexBase;
            vkCmdBindVertexBuffers(commandBuffer, 0, 1, &sortedVertices->buffer, &vertexBase);
            vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.fencesColoredLights.pipelineLayout, 1, 1, &sortedAttributesResources.descriptorSet, 0, nullptr);
            if (appState.Scene.sortedIndices16Size > 0)
            {
                vkCmdBindIndexBuffer(commandBuffer, sortedIndices16->buffer, appState.Scene.fencesColoredLights.indexBase, VK_INDEX_TYPE_UINT16);
            }
            else
            {
                vkCmdBindIndexBuffer(commandBuffer, sortedIndices32->buffer, appState.Scene.fencesColoredLights.indexBase, VK_INDEX_TYPE_UINT32);
            }
            SetTintPushConstants(pushConstants);
            vkCmdPushConstants(commandBuffer, appState.Scene.fencesColoredLights.pipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, 5 * sizeof(float), &pushConstants);
			appState.Scene.fencesColoredLights.Render(commandBuffer);
#if !defined(NDEBUG) || defined(ENABLE_DEBUG_UTILS)
			appState.vkCmdEndDebugUtilsLabelEXT(commandBuffer);
#endif
        }
        if (appState.Scene.fencesRGBA.last >= 0)
        {
#if !defined(NDEBUG) || defined(ENABLE_DEBUG_UTILS)
			renderLabel.pLabelName = FENCES_RGBA_NAME;
			appState.vkCmdBeginDebugUtilsLabelEXT(commandBuffer, &renderLabel);
#endif
            vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.fencesRGBA.pipeline);
            vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.fencesRGBA.pipelineLayout, 0, 1, &sceneMatricesResources.descriptorSet, 0, nullptr);
            auto vertexBase = appState.Scene.fencesRGBA.vertexBase;
            vkCmdBindVertexBuffers(commandBuffer, 0, 1, &sortedVertices->buffer, &vertexBase);
            vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.fencesRGBA.pipelineLayout, 1, 1, &sortedAttributesResources.descriptorSet, 0, nullptr);
            if (appState.Scene.sortedIndices16Size > 0)
            {
                vkCmdBindIndexBuffer(commandBuffer, sortedIndices16->buffer, appState.Scene.fencesRGBA.indexBase, VK_INDEX_TYPE_UINT16);
            }
            else
            {
                vkCmdBindIndexBuffer(commandBuffer, sortedIndices32->buffer, appState.Scene.fencesRGBA.indexBase, VK_INDEX_TYPE_UINT32);
            }
            SetTintPushConstants(pushConstants);
            vkCmdPushConstants(commandBuffer, appState.Scene.fencesRGBA.pipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, 5 * sizeof(float), &pushConstants);
			appState.Scene.fencesRGBA.Render(commandBuffer);
#if !defined(NDEBUG) || defined(ENABLE_DEBUG_UTILS)
			appState.vkCmdEndDebugUtilsLabelEXT(commandBuffer);
#endif
        }
        if (appState.Scene.fencesRGBAColoredLights.last >= 0)
        {
#if !defined(NDEBUG) || defined(ENABLE_DEBUG_UTILS)
			renderLabel.pLabelName = FENCES_RGBA_COLORED_LIGHTS_NAME;
			appState.vkCmdBeginDebugUtilsLabelEXT(commandBuffer, &renderLabel);
#endif
            vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.fencesRGBAColoredLights.pipeline);
            vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.fencesRGBAColoredLights.pipelineLayout, 0, 1, &sceneMatricesResources.descriptorSet, 0, nullptr);
            auto vertexBase = appState.Scene.fencesRGBAColoredLights.vertexBase;
            vkCmdBindVertexBuffers(commandBuffer, 0, 1, &sortedVertices->buffer, &vertexBase);
            vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.fencesRGBAColoredLights.pipelineLayout, 1, 1, &sortedAttributesResources.descriptorSet, 0, nullptr);
            if (appState.Scene.sortedIndices16Size > 0)
            {
                vkCmdBindIndexBuffer(commandBuffer, sortedIndices16->buffer, appState.Scene.fencesRGBAColoredLights.indexBase, VK_INDEX_TYPE_UINT16);
            }
            else
            {
                vkCmdBindIndexBuffer(commandBuffer, sortedIndices32->buffer, appState.Scene.fencesRGBAColoredLights.indexBase, VK_INDEX_TYPE_UINT32);
            }
            SetTintPushConstants(pushConstants);
            vkCmdPushConstants(commandBuffer, appState.Scene.fencesRGBAColoredLights.pipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, 5 * sizeof(float), &pushConstants);
			appState.Scene.fencesRGBAColoredLights.Render(commandBuffer);
#if !defined(NDEBUG) || defined(ENABLE_DEBUG_UTILS)
			appState.vkCmdEndDebugUtilsLabelEXT(commandBuffer);
#endif
        }
        if (appState.Scene.fencesRGBANoGlow.last >= 0)
        {
#if !defined(NDEBUG) || defined(ENABLE_DEBUG_UTILS)
			renderLabel.pLabelName = FENCES_RGBA_NO_GLOW_NAME;
			appState.vkCmdBeginDebugUtilsLabelEXT(commandBuffer, &renderLabel);
#endif
            vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.fencesRGBANoGlow.pipeline);
            vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.fencesRGBANoGlow.pipelineLayout, 0, 1, &sceneMatricesResources.descriptorSet, 0, nullptr);
            auto vertexBase = appState.Scene.fencesRGBANoGlow.vertexBase;
            vkCmdBindVertexBuffers(commandBuffer, 0, 1, &sortedVertices->buffer, &vertexBase);
            vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.fencesRGBANoGlow.pipelineLayout, 1, 1, &sortedAttributesResources.descriptorSet, 0, nullptr);
            if (appState.Scene.sortedIndices16Size > 0)
            {
                vkCmdBindIndexBuffer(commandBuffer, sortedIndices16->buffer, appState.Scene.fencesRGBANoGlow.indexBase, VK_INDEX_TYPE_UINT16);
            }
            else
            {
                vkCmdBindIndexBuffer(commandBuffer, sortedIndices32->buffer, appState.Scene.fencesRGBANoGlow.indexBase, VK_INDEX_TYPE_UINT32);
            }
            SetTintPushConstants(pushConstants);
            vkCmdPushConstants(commandBuffer, appState.Scene.fencesRGBANoGlow.pipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, 5 * sizeof(float), &pushConstants);
			appState.Scene.fencesRGBANoGlow.Render(commandBuffer);
#if !defined(NDEBUG) || defined(ENABLE_DEBUG_UTILS)
			appState.vkCmdEndDebugUtilsLabelEXT(commandBuffer);
#endif
        }
        if (appState.Scene.fencesRGBANoGlowColoredLights.last >= 0)
        {
#if !defined(NDEBUG) || defined(ENABLE_DEBUG_UTILS)
			renderLabel.pLabelName = FENCES_RGBA_NO_GLOW_COLORED_LIGHTS_NAME;
			appState.vkCmdBeginDebugUtilsLabelEXT(commandBuffer, &renderLabel);
#endif
            vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.fencesRGBANoGlowColoredLights.pipeline);
            vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.fencesRGBANoGlowColoredLights.pipelineLayout, 0, 1, &sceneMatricesResources.descriptorSet, 0, nullptr);
            auto vertexBase = appState.Scene.fencesRGBANoGlowColoredLights.vertexBase;
            vkCmdBindVertexBuffers(commandBuffer, 0, 1, &sortedVertices->buffer, &vertexBase);
            vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.fencesRGBANoGlowColoredLights.pipelineLayout, 1, 1, &sortedAttributesResources.descriptorSet, 0, nullptr);
            if (appState.Scene.sortedIndices16Size > 0)
            {
                vkCmdBindIndexBuffer(commandBuffer, sortedIndices16->buffer, appState.Scene.fencesRGBANoGlowColoredLights.indexBase, VK_INDEX_TYPE_UINT16);
            }
            else
            {
                vkCmdBindIndexBuffer(commandBuffer, sortedIndices32->buffer, appState.Scene.fencesRGBANoGlowColoredLights.indexBase, VK_INDEX_TYPE_UINT32);
            }
            SetTintPushConstants(pushConstants);
            vkCmdPushConstants(commandBuffer, appState.Scene.fencesRGBANoGlowColoredLights.pipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, 5 * sizeof(float), &pushConstants);
			appState.Scene.fencesRGBANoGlowColoredLights.Render(commandBuffer);
#if !defined(NDEBUG) || defined(ENABLE_DEBUG_UTILS)
			appState.vkCmdEndDebugUtilsLabelEXT(commandBuffer);
#endif
        }
        if (appState.Scene.fencesRotated.last >= 0)
        {
#if !defined(NDEBUG) || defined(ENABLE_DEBUG_UTILS)
			renderLabel.pLabelName = FENCES_ROTATED_NAME;
			appState.vkCmdBeginDebugUtilsLabelEXT(commandBuffer, &renderLabel);
#endif
            vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.fencesRotated.pipeline);
            vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.fencesRotated.pipelineLayout, 0, 1, &sceneMatricesAndColormapResources.descriptorSet, 0, nullptr);
            auto vertexBase = appState.Scene.fencesRotated.vertexBase;
            vkCmdBindVertexBuffers(commandBuffer, 0, 1, &sortedVertices->buffer, &vertexBase);
            vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.fencesRotated.pipelineLayout, 1, 1, &sortedAttributesResources.descriptorSet, 0, nullptr);
            if (appState.Scene.sortedIndices16Size > 0)
            {
                vkCmdBindIndexBuffer(commandBuffer, sortedIndices16->buffer, appState.Scene.fencesRotated.indexBase, VK_INDEX_TYPE_UINT16);
            }
            else
            {
                vkCmdBindIndexBuffer(commandBuffer, sortedIndices32->buffer, appState.Scene.fencesRotated.indexBase, VK_INDEX_TYPE_UINT32);
            }
			appState.Scene.fencesRotated.Render(commandBuffer);
#if !defined(NDEBUG) || defined(ENABLE_DEBUG_UTILS)
			appState.vkCmdEndDebugUtilsLabelEXT(commandBuffer);
#endif
        }
        if (appState.Scene.fencesRotatedColoredLights.last >= 0)
        {
#if !defined(NDEBUG) || defined(ENABLE_DEBUG_UTILS)
			renderLabel.pLabelName = FENCES_ROTATED_COLORED_LIGHTS_NAME;
			appState.vkCmdBeginDebugUtilsLabelEXT(commandBuffer, &renderLabel);
#endif
            vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.fencesRotatedColoredLights.pipeline);
            vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.fencesRotatedColoredLights.pipelineLayout, 0, 1, &sceneMatricesAndNeutralPaletteResources.descriptorSet, 0, nullptr);
            auto vertexBase = appState.Scene.fencesRotatedColoredLights.vertexBase;
            vkCmdBindVertexBuffers(commandBuffer, 0, 1, &sortedVertices->buffer, &vertexBase);
            vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.fencesRotatedColoredLights.pipelineLayout, 1, 1, &sortedAttributesResources.descriptorSet, 0, nullptr);
            if (appState.Scene.sortedIndices16Size > 0)
            {
                vkCmdBindIndexBuffer(commandBuffer, sortedIndices16->buffer, appState.Scene.fencesRotatedColoredLights.indexBase, VK_INDEX_TYPE_UINT16);
            }
            else
            {
                vkCmdBindIndexBuffer(commandBuffer, sortedIndices32->buffer, appState.Scene.fencesRotatedColoredLights.indexBase, VK_INDEX_TYPE_UINT32);
            }
            SetTintPushConstants(pushConstants);
            vkCmdPushConstants(commandBuffer, appState.Scene.fencesRotatedColoredLights.pipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, 5 * sizeof(float), &pushConstants);
			appState.Scene.fencesRotatedColoredLights.Render(commandBuffer);
#if !defined(NDEBUG) || defined(ENABLE_DEBUG_UTILS)
			appState.vkCmdEndDebugUtilsLabelEXT(commandBuffer);
#endif
        }
        if (appState.Scene.fencesRotatedRGBA.last >= 0)
        {
#if !defined(NDEBUG) || defined(ENABLE_DEBUG_UTILS)
			renderLabel.pLabelName = FENCES_ROTATED_RGBA_NAME;
			appState.vkCmdBeginDebugUtilsLabelEXT(commandBuffer, &renderLabel);
#endif
            vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.fencesRotatedRGBA.pipeline);
            vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.fencesRotatedRGBA.pipelineLayout, 0, 1, &sceneMatricesResources.descriptorSet, 0, nullptr);
            auto vertexBase = appState.Scene.fencesRotatedRGBA.vertexBase;
            vkCmdBindVertexBuffers(commandBuffer, 0, 1, &sortedVertices->buffer, &vertexBase);
            vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.fencesRotatedRGBA.pipelineLayout, 1, 1, &sortedAttributesResources.descriptorSet, 0, nullptr);
            if (appState.Scene.sortedIndices16Size > 0)
            {
                vkCmdBindIndexBuffer(commandBuffer, sortedIndices16->buffer, appState.Scene.fencesRotatedRGBA.indexBase, VK_INDEX_TYPE_UINT16);
            }
            else
            {
                vkCmdBindIndexBuffer(commandBuffer, sortedIndices32->buffer, appState.Scene.fencesRotatedRGBA.indexBase, VK_INDEX_TYPE_UINT32);
            }
            SetTintPushConstants(pushConstants);
            vkCmdPushConstants(commandBuffer, appState.Scene.fencesRotatedRGBA.pipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, 5 * sizeof(float), &pushConstants);
			appState.Scene.fencesRotatedRGBA.Render(commandBuffer);
#if !defined(NDEBUG) || defined(ENABLE_DEBUG_UTILS)
			appState.vkCmdEndDebugUtilsLabelEXT(commandBuffer);
#endif
        }
        if (appState.Scene.fencesRotatedRGBAColoredLights.last >= 0)
        {
#if !defined(NDEBUG) || defined(ENABLE_DEBUG_UTILS)
			renderLabel.pLabelName = FENCES_ROTATED_RGBA_COLORED_LIGHTS_NAME;
			appState.vkCmdBeginDebugUtilsLabelEXT(commandBuffer, &renderLabel);
#endif
            vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.fencesRotatedRGBAColoredLights.pipeline);
            vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.fencesRotatedRGBAColoredLights.pipelineLayout, 0, 1, &sceneMatricesResources.descriptorSet, 0, nullptr);
            auto vertexBase = appState.Scene.fencesRotatedRGBAColoredLights.vertexBase;
            vkCmdBindVertexBuffers(commandBuffer, 0, 1, &sortedVertices->buffer, &vertexBase);
            vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.fencesRotatedRGBAColoredLights.pipelineLayout, 1, 1, &sortedAttributesResources.descriptorSet, 0, nullptr);
            if (appState.Scene.sortedIndices16Size > 0)
            {
                vkCmdBindIndexBuffer(commandBuffer, sortedIndices16->buffer, appState.Scene.fencesRotatedRGBAColoredLights.indexBase, VK_INDEX_TYPE_UINT16);
            }
            else
            {
                vkCmdBindIndexBuffer(commandBuffer, sortedIndices32->buffer, appState.Scene.fencesRotatedRGBAColoredLights.indexBase, VK_INDEX_TYPE_UINT32);
            }
            SetTintPushConstants(pushConstants);
            vkCmdPushConstants(commandBuffer, appState.Scene.fencesRotatedRGBAColoredLights.pipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, 5 * sizeof(float), &pushConstants);
			appState.Scene.fencesRotatedRGBAColoredLights.Render(commandBuffer);
#if !defined(NDEBUG) || defined(ENABLE_DEBUG_UTILS)
			appState.vkCmdEndDebugUtilsLabelEXT(commandBuffer);
#endif
        }
        if (appState.Scene.fencesRotatedRGBANoGlow.last >= 0)
        {
#if !defined(NDEBUG) || defined(ENABLE_DEBUG_UTILS)
			renderLabel.pLabelName = FENCES_ROTATED_RGBA_NO_GLOW_NAME;
			appState.vkCmdBeginDebugUtilsLabelEXT(commandBuffer, &renderLabel);
#endif
            vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.fencesRotatedRGBANoGlow.pipeline);
            vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.fencesRotatedRGBANoGlow.pipelineLayout, 0, 1, &sceneMatricesResources.descriptorSet, 0, nullptr);
            auto vertexBase = appState.Scene.fencesRotatedRGBANoGlow.vertexBase;
            vkCmdBindVertexBuffers(commandBuffer, 0, 1, &sortedVertices->buffer, &vertexBase);
            vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.fencesRotatedRGBANoGlow.pipelineLayout, 1, 1, &sortedAttributesResources.descriptorSet, 0, nullptr);
            if (appState.Scene.sortedIndices16Size > 0)
            {
                vkCmdBindIndexBuffer(commandBuffer, sortedIndices16->buffer, appState.Scene.fencesRotatedRGBANoGlow.indexBase, VK_INDEX_TYPE_UINT16);
            }
            else
            {
                vkCmdBindIndexBuffer(commandBuffer, sortedIndices32->buffer, appState.Scene.fencesRotatedRGBANoGlow.indexBase, VK_INDEX_TYPE_UINT32);
            }
            SetTintPushConstants(pushConstants);
            vkCmdPushConstants(commandBuffer, appState.Scene.fencesRotatedRGBANoGlow.pipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, 5 * sizeof(float), &pushConstants);
			appState.Scene.fencesRotatedRGBANoGlow.Render(commandBuffer);
#if !defined(NDEBUG) || defined(ENABLE_DEBUG_UTILS)
			appState.vkCmdEndDebugUtilsLabelEXT(commandBuffer);
#endif
        }
        if (appState.Scene.fencesRotatedRGBANoGlowColoredLights.last >= 0)
        {
#if !defined(NDEBUG) || defined(ENABLE_DEBUG_UTILS)
			renderLabel.pLabelName = FENCES_ROTATED_RGBA_NO_GLOW_COLORED_LIGHTS_NAME;
			appState.vkCmdBeginDebugUtilsLabelEXT(commandBuffer, &renderLabel);
#endif
            vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.fencesRotatedRGBANoGlowColoredLights.pipeline);
            vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.fencesRotatedRGBANoGlowColoredLights.pipelineLayout, 0, 1, &sceneMatricesResources.descriptorSet, 0, nullptr);
            auto vertexBase = appState.Scene.fencesRotatedRGBANoGlowColoredLights.vertexBase;
            vkCmdBindVertexBuffers(commandBuffer, 0, 1, &sortedVertices->buffer, &vertexBase);
            vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.fencesRotatedRGBANoGlowColoredLights.pipelineLayout, 1, 1, &sortedAttributesResources.descriptorSet, 0, nullptr);
            if (appState.Scene.sortedIndices16Size > 0)
            {
                vkCmdBindIndexBuffer(commandBuffer, sortedIndices16->buffer, appState.Scene.fencesRotatedRGBANoGlowColoredLights.indexBase, VK_INDEX_TYPE_UINT16);
            }
            else
            {
                vkCmdBindIndexBuffer(commandBuffer, sortedIndices32->buffer, appState.Scene.fencesRotatedRGBANoGlowColoredLights.indexBase, VK_INDEX_TYPE_UINT32);
            }
            SetTintPushConstants(pushConstants);
            vkCmdPushConstants(commandBuffer, appState.Scene.fencesRotatedRGBANoGlowColoredLights.pipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, 5 * sizeof(float), &pushConstants);
			appState.Scene.fencesRotatedRGBANoGlowColoredLights.Render(commandBuffer);
#if !defined(NDEBUG) || defined(ENABLE_DEBUG_UTILS)
			appState.vkCmdEndDebugUtilsLabelEXT(commandBuffer);
#endif
        }
		if (appState.Scene.turbulentRotated.last >= 0)
		{
#if !defined(NDEBUG) || defined(ENABLE_DEBUG_UTILS)
			renderLabel.pLabelName = TURBULENT_ROTATED_NAME;
			appState.vkCmdBeginDebugUtilsLabelEXT(commandBuffer, &renderLabel);
#endif
			vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.turbulentRotated.pipeline);
			vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.turbulentRotated.pipelineLayout, 0, 1, &sceneMatricesAndPaletteResources.descriptorSet, 0, nullptr);
			auto vertexBase = appState.Scene.turbulentRotated.vertexBase;
			vkCmdBindVertexBuffers(commandBuffer, 0, 1, &sortedVertices->buffer, &vertexBase);
            vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.turbulentRotated.pipelineLayout, 1, 1, &sortedAttributesResources.descriptorSet, 0, nullptr);
			if (appState.Scene.sortedIndices16Size > 0)
			{
				vkCmdBindIndexBuffer(commandBuffer, sortedIndices16->buffer, appState.Scene.turbulentRotated.indexBase, VK_INDEX_TYPE_UINT16);
			}
			else
			{
				vkCmdBindIndexBuffer(commandBuffer, sortedIndices32->buffer, appState.Scene.turbulentRotated.indexBase, VK_INDEX_TYPE_UINT32);
			}
			auto time = (float)appState.FromEngine.time;
			vkCmdPushConstants(commandBuffer, appState.Scene.turbulentRotated.pipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(float), &time);
			VkDeviceSize indexBase = 0;
            for (auto t = 0; t < appState.Scene.turbulentRotated.sorted.count; t++)
            {
                const auto& texture = appState.Scene.turbulentRotated.sorted.textures[t];
				vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.turbulentRotated.pipelineLayout, 2, 1, &texture.texture, 0, nullptr);
				vkCmdDrawIndexed(commandBuffer, texture.indexCount, 1, indexBase, 0, 0);
				indexBase += texture.indexCount;
			}
#if !defined(NDEBUG) || defined(ENABLE_DEBUG_UTILS)
			appState.vkCmdEndDebugUtilsLabelEXT(commandBuffer);
#endif
		}
		if (appState.Scene.turbulentRotatedRGBA.last >= 0)
		{
#if !defined(NDEBUG) || defined(ENABLE_DEBUG_UTILS)
			renderLabel.pLabelName = TURBULENT_ROTATED_RGBA_NAME;
			appState.vkCmdBeginDebugUtilsLabelEXT(commandBuffer, &renderLabel);
#endif
			vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.turbulentRotatedRGBA.pipeline);
			vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.turbulentRotatedRGBA.pipelineLayout, 0, 1, &sceneMatricesResources.descriptorSet, 0, nullptr);
			auto vertexBase = appState.Scene.turbulentRotatedRGBA.vertexBase;
			vkCmdBindVertexBuffers(commandBuffer, 0, 1, &sortedVertices->buffer, &vertexBase);
            vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.turbulentRotatedRGBA.pipelineLayout, 1, 1, &sortedAttributesResources.descriptorSet, 0, nullptr);
			if (appState.Scene.sortedIndices16Size > 0)
			{
				vkCmdBindIndexBuffer(commandBuffer, sortedIndices16->buffer, appState.Scene.turbulentRotatedRGBA.indexBase, VK_INDEX_TYPE_UINT16);
			}
			else
			{
				vkCmdBindIndexBuffer(commandBuffer, sortedIndices32->buffer, appState.Scene.turbulentRotatedRGBA.indexBase, VK_INDEX_TYPE_UINT32);
			}
			SetTintPushConstants(pushConstants);
			pushConstants[5] = (float)appState.FromEngine.time;
			vkCmdPushConstants(commandBuffer, appState.Scene.turbulentRotatedRGBA.pipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, 6 * sizeof(float), pushConstants);
			VkDeviceSize indexBase = 0;
            for (auto t = 0; t < appState.Scene.turbulentRotatedRGBA.sorted.count; t++)
            {
                const auto& texture = appState.Scene.turbulentRotatedRGBA.sorted.textures[t];
				vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.turbulentRotatedRGBA.pipelineLayout, 2, 1, &texture.texture, 0, nullptr);
				vkCmdDrawIndexed(commandBuffer, texture.indexCount, 1, indexBase, 0, 0);
				indexBase += texture.indexCount;
			}
#if !defined(NDEBUG) || defined(ENABLE_DEBUG_UTILS)
			appState.vkCmdEndDebugUtilsLabelEXT(commandBuffer);
#endif
		}
		if (appState.Scene.turbulentRotatedLit.last >= 0)
		{
#if !defined(NDEBUG) || defined(ENABLE_DEBUG_UTILS)
			renderLabel.pLabelName = TURBULENT_ROTATED_LIT_NAME;
			appState.vkCmdBeginDebugUtilsLabelEXT(commandBuffer, &renderLabel);
#endif
			vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.turbulentRotatedLit.pipeline);
			vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.turbulentRotatedLit.pipelineLayout, 0, 1, &sceneMatricesAndColormapResources.descriptorSet, 0, nullptr);
			auto vertexBase = appState.Scene.turbulentRotatedLit.vertexBase;
			vkCmdBindVertexBuffers(commandBuffer, 0, 1, &sortedVertices->buffer, &vertexBase);
            vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.turbulentRotatedLit.pipelineLayout, 1, 1, &sortedAttributesResources.descriptorSet, 0, nullptr);
			if (appState.Scene.sortedIndices16Size > 0)
			{
				vkCmdBindIndexBuffer(commandBuffer, sortedIndices16->buffer, appState.Scene.turbulentRotatedLit.indexBase, VK_INDEX_TYPE_UINT16);
			}
			else
			{
				vkCmdBindIndexBuffer(commandBuffer, sortedIndices32->buffer, appState.Scene.turbulentRotatedLit.indexBase, VK_INDEX_TYPE_UINT32);
			}
			auto time = (float)appState.FromEngine.time;
			vkCmdPushConstants(commandBuffer, appState.Scene.turbulentRotatedLit.pipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(float), &time);
			appState.Scene.turbulentRotatedLit.Render(commandBuffer);
#if !defined(NDEBUG) || defined(ENABLE_DEBUG_UTILS)
			appState.vkCmdEndDebugUtilsLabelEXT(commandBuffer);
#endif
		}
		if (appState.Scene.turbulentRotatedColoredLights.last >= 0)
		{
#if !defined(NDEBUG) || defined(ENABLE_DEBUG_UTILS)
			renderLabel.pLabelName = TURBULENT_ROTATED_COLORED_LIGHTS_NAME;
			appState.vkCmdBeginDebugUtilsLabelEXT(commandBuffer, &renderLabel);
#endif
			vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.turbulentRotatedColoredLights.pipeline);
			vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.turbulentRotatedColoredLights.pipelineLayout, 0, 1, &sceneMatricesAndNeutralPaletteResources.descriptorSet, 0, nullptr);
			auto vertexBase = appState.Scene.turbulentRotatedColoredLights.vertexBase;
			vkCmdBindVertexBuffers(commandBuffer, 0, 1, &sortedVertices->buffer, &vertexBase);
            vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.turbulentRotatedColoredLights.pipelineLayout, 1, 1, &sortedAttributesResources.descriptorSet, 0, nullptr);
			if (appState.Scene.sortedIndices16Size > 0)
			{
				vkCmdBindIndexBuffer(commandBuffer, sortedIndices16->buffer, appState.Scene.turbulentRotatedColoredLights.indexBase, VK_INDEX_TYPE_UINT16);
			}
			else
			{
				vkCmdBindIndexBuffer(commandBuffer, sortedIndices32->buffer, appState.Scene.turbulentRotatedColoredLights.indexBase, VK_INDEX_TYPE_UINT32);
			}
			SetTintPushConstants(pushConstants);
			pushConstants[5] = (float)appState.FromEngine.time;
			vkCmdPushConstants(commandBuffer, appState.Scene.turbulentRotatedColoredLights.pipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, 6 * sizeof(float), pushConstants);
			appState.Scene.turbulentRotatedColoredLights.Render(commandBuffer);
#if !defined(NDEBUG) || defined(ENABLE_DEBUG_UTILS)
			appState.vkCmdEndDebugUtilsLabelEXT(commandBuffer);
#endif
		}
		if (appState.Scene.turbulentRotatedRGBALit.last >= 0)
		{
#if !defined(NDEBUG) || defined(ENABLE_DEBUG_UTILS)
			renderLabel.pLabelName = TURBULENT_ROTATED_RGBA_LIT_NAME;
			appState.vkCmdBeginDebugUtilsLabelEXT(commandBuffer, &renderLabel);
#endif
			vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.turbulentRotatedRGBALit.pipeline);
			vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.turbulentRotatedRGBALit.pipelineLayout, 0, 1, &sceneMatricesResources.descriptorSet, 0, nullptr);
			auto vertexBase = appState.Scene.turbulentRotatedRGBALit.vertexBase;
			vkCmdBindVertexBuffers(commandBuffer, 0, 1, &sortedVertices->buffer, &vertexBase);
            vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.turbulentRotatedRGBALit.pipelineLayout, 1, 1, &sortedAttributesResources.descriptorSet, 0, nullptr);
			if (appState.Scene.sortedIndices16Size > 0)
			{
				vkCmdBindIndexBuffer(commandBuffer, sortedIndices16->buffer, appState.Scene.turbulentRotatedRGBALit.indexBase, VK_INDEX_TYPE_UINT16);
			}
			else
			{
				vkCmdBindIndexBuffer(commandBuffer, sortedIndices32->buffer, appState.Scene.turbulentRotatedRGBALit.indexBase, VK_INDEX_TYPE_UINT32);
			}
			SetTintPushConstants(pushConstants);
			pushConstants[5] = (float)appState.FromEngine.time;
			vkCmdPushConstants(commandBuffer, appState.Scene.turbulentRotatedRGBALit.pipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, 6 * sizeof(float), pushConstants);
			appState.Scene.turbulentRotatedRGBALit.Render(commandBuffer);
#if !defined(NDEBUG) || defined(ENABLE_DEBUG_UTILS)
			appState.vkCmdEndDebugUtilsLabelEXT(commandBuffer);
#endif
		}
		if (appState.Scene.turbulentRotatedRGBAColoredLights.last >= 0)
		{
#if !defined(NDEBUG) || defined(ENABLE_DEBUG_UTILS)
			renderLabel.pLabelName = TURBULENT_ROTATED_RGBA_COLORED_LIGHTS_NAME;
			appState.vkCmdBeginDebugUtilsLabelEXT(commandBuffer, &renderLabel);
#endif
			vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.turbulentRotatedRGBAColoredLights.pipeline);
			vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.turbulentRotatedRGBAColoredLights.pipelineLayout, 0, 1, &sceneMatricesResources.descriptorSet, 0, nullptr);
			auto vertexBase = appState.Scene.turbulentRotatedRGBAColoredLights.vertexBase;
			vkCmdBindVertexBuffers(commandBuffer, 0, 1, &sortedVertices->buffer, &vertexBase);
            vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.turbulentRotatedRGBAColoredLights.pipelineLayout, 1, 1, &sortedAttributesResources.descriptorSet, 0, nullptr);
			if (appState.Scene.sortedIndices16Size > 0)
			{
				vkCmdBindIndexBuffer(commandBuffer, sortedIndices16->buffer, appState.Scene.turbulentRotatedRGBAColoredLights.indexBase, VK_INDEX_TYPE_UINT16);
			}
			else
			{
				vkCmdBindIndexBuffer(commandBuffer, sortedIndices32->buffer, appState.Scene.turbulentRotatedRGBAColoredLights.indexBase, VK_INDEX_TYPE_UINT32);
			}
			SetTintPushConstants(pushConstants);
			pushConstants[5] = (float)appState.FromEngine.time;
			vkCmdPushConstants(commandBuffer, appState.Scene.turbulentRotatedRGBAColoredLights.pipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, 6 * sizeof(float), pushConstants);
			appState.Scene.turbulentRotatedRGBAColoredLights.Render(commandBuffer);
#if !defined(NDEBUG) || defined(ENABLE_DEBUG_UTILS)
			appState.vkCmdEndDebugUtilsLabelEXT(commandBuffer);
#endif
		}
		if (appState.Scene.sprites.last >= 0)
		{
#if !defined(NDEBUG) || defined(ENABLE_DEBUG_UTILS)
			renderLabel.pLabelName = SPRITES_NAME;
			appState.vkCmdBeginDebugUtilsLabelEXT(commandBuffer, &renderLabel);
#endif
			vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.sprites.pipeline);
			vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.sprites.pipelineLayout, 0, 1, &sceneMatricesAndPaletteResources.descriptorSet, 0, nullptr);
			auto vertexBase = appState.Scene.sprites.vertexBase;
			vkCmdBindVertexBuffers(commandBuffer, 0, 1, &sortedVertices->buffer, &vertexBase);
			if (appState.Scene.sortedIndices16Size > 0)
			{
				vkCmdBindIndexBuffer(commandBuffer, sortedIndices16->buffer, appState.Scene.sprites.indexBase, VK_INDEX_TYPE_UINT16);
			}
			else
			{
				vkCmdBindIndexBuffer(commandBuffer, sortedIndices32->buffer, appState.Scene.sprites.indexBase, VK_INDEX_TYPE_UINT32);
			}
			VkDeviceSize indexBase = 0;
			for (auto t = 0; t < appState.Scene.sprites.sorted.count; t++)
			{
				const auto& texture = appState.Scene.sprites.sorted.textures[t];
				vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.sprites.pipelineLayout, 1, 1, &texture.texture, 0, nullptr);
				vkCmdDrawIndexed(commandBuffer, texture.indexCount, 1, indexBase, 0, 0);
				indexBase += texture.indexCount;
			}
#if !defined(NDEBUG) || defined(ENABLE_DEBUG_UTILS)
			appState.vkCmdEndDebugUtilsLabelEXT(commandBuffer);
#endif
		}
		if (appState.Scene.aliasAlpha.last >= 0)
		{
#if !defined(NDEBUG) || defined(ENABLE_DEBUG_UTILS)
			renderLabel.pLabelName = ALIAS_ALPHA_NAME;
			appState.vkCmdBeginDebugUtilsLabelEXT(commandBuffer, &renderLabel);
#endif
			vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.aliasAlpha.pipeline);
			vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.aliasAlpha.pipelineLayout, 0, 1, &sceneMatricesAndPaletteResources.descriptorSet, 0, nullptr);
			pushConstants[3] = 0;
			pushConstants[7] = 0;
			pushConstants[11] = 0;
			pushConstants[15] = 1;
			SharedMemoryBuffer* previousVertices = nullptr;
			SharedMemoryBuffer* previousTexCoords = nullptr;
			VkDescriptorSet previousColormapDescriptorSet = VK_NULL_HANDLE;
			SharedMemoryTexture* previousTexture = nullptr;
			SharedMemoryBuffer* previousIndices = nullptr;
			for (auto i = 0; i <= appState.Scene.aliasAlpha.last; i++)
			{
				auto& loaded = appState.Scene.aliasAlpha.loaded[i];
				auto vertices = loaded.vertices.buffer;
				if (previousVertices != vertices)
				{
					vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertices->buffer, &appState.NoOffset);
					previousVertices = vertices;
				}
				auto texCoords = loaded.texCoords.buffer;
				if (previousTexCoords != texCoords)
				{
					vkCmdBindVertexBuffers(commandBuffer, 1, 1, &texCoords->buffer, &appState.NoOffset);
					previousTexCoords = texCoords;
				}
				VkDeviceSize attributeOffset = aliasAttributeBase + loaded.firstAttribute * sizeof(float);
				vkCmdBindVertexBuffers(commandBuffer, 2, 1, &attributes->buffer, &attributeOffset);
				SetPushConstants(loaded, pushConstants);
				vkCmdPushConstants(commandBuffer, appState.Scene.aliasAlpha.pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, 16 * sizeof(float), pushConstants);
				if (loaded.isHostColormap)
				{
					if (previousColormapDescriptorSet != host_colormapResources.descriptorSet)
					{
						vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.aliasAlpha.pipelineLayout, 1, 1, &host_colormapResources.descriptorSet, 0, nullptr);
						previousColormapDescriptorSet = host_colormapResources.descriptorSet;
					}
				}
				else
				{
					auto colormap = loaded.colormap.texture;
					if (colormapResources.bound[descriptorSetIndex] != colormap)
					{
						textureInfo.sampler = appState.Scene.sampler;
						textureInfo.imageView = colormap->view;
						writes[0].dstSet = colormapResources.descriptorSets[descriptorSetIndex];
						vkUpdateDescriptorSets(appState.Device, 1, writes, 0, nullptr);
						colormapResources.bound[descriptorSetIndex] = colormap;
					}
					vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.aliasAlpha.pipelineLayout, 1, 1, &colormapResources.descriptorSets[descriptorSetIndex], 0, nullptr);
					previousColormapDescriptorSet = colormapResources.descriptorSets[descriptorSetIndex];
					descriptorSetIndex++;
				}
				auto texture = loaded.texture.texture;
				if (previousTexture != texture)
				{
					vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.aliasAlpha.pipelineLayout, 2, 1, &texture->descriptorSet, 0, nullptr);
					previousTexture = texture;
				}
				auto indices = loaded.indices.indices.buffer;
				if (previousIndices != indices)
				{
					vkCmdBindIndexBuffer(commandBuffer, indices->buffer, 0, loaded.indices.indices.indexType);
					previousIndices = indices;
				}
				vkCmdDrawIndexed(commandBuffer, loaded.count, 1, loaded.indices.indices.firstIndex, 0, 0);
			}
#if !defined(NDEBUG) || defined(ENABLE_DEBUG_UTILS)
			appState.vkCmdEndDebugUtilsLabelEXT(commandBuffer);
#endif
		}
		if (appState.Scene.aliasHoley.last >= 0)
		{
#if !defined(NDEBUG) || defined(ENABLE_DEBUG_UTILS)
			renderLabel.pLabelName = ALIAS_HOLEY_NAME;
			appState.vkCmdBeginDebugUtilsLabelEXT(commandBuffer, &renderLabel);
#endif
			vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.aliasHoley.pipeline);
			vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.aliasHoley.pipelineLayout, 0, 1, &sceneMatricesAndPaletteResources.descriptorSet, 0, nullptr);
			pushConstants[3] = 0;
			pushConstants[7] = 0;
			pushConstants[11] = 0;
			pushConstants[15] = 1;
			SharedMemoryBuffer* previousVertices = nullptr;
			SharedMemoryBuffer* previousTexCoords = nullptr;
			VkDescriptorSet previousColormapDescriptorSet = VK_NULL_HANDLE;
			SharedMemoryTexture* previousTexture = nullptr;
			SharedMemoryBuffer* previousIndices = nullptr;
			for (auto i = 0; i <= appState.Scene.aliasHoley.last; i++)
			{
				auto& loaded = appState.Scene.aliasHoley.loaded[i];
				auto vertices = loaded.vertices.buffer;
				if (previousVertices != vertices)
				{
					vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertices->buffer, &appState.NoOffset);
					previousVertices = vertices;
				}
				auto texCoords = loaded.texCoords.buffer;
				if (previousTexCoords != texCoords)
				{
					vkCmdBindVertexBuffers(commandBuffer, 1, 1, &texCoords->buffer, &appState.NoOffset);
					previousTexCoords = texCoords;
				}
				VkDeviceSize attributeOffset = aliasAttributeBase + loaded.firstAttribute * sizeof(float);
				vkCmdBindVertexBuffers(commandBuffer, 2, 1, &attributes->buffer, &attributeOffset);
				SetPushConstants(loaded, pushConstants);
				vkCmdPushConstants(commandBuffer, appState.Scene.aliasHoley.pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, 16 * sizeof(float), pushConstants);
				if (loaded.isHostColormap)
				{
					if (previousColormapDescriptorSet != host_colormapResources.descriptorSet)
					{
						vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.aliasHoley.pipelineLayout, 1, 1, &host_colormapResources.descriptorSet, 0, nullptr);
						previousColormapDescriptorSet = host_colormapResources.descriptorSet;
					}
				}
				else
				{
					auto colormap = loaded.colormap.texture;
					if (colormapResources.bound[descriptorSetIndex] != colormap)
					{
						textureInfo.sampler = appState.Scene.sampler;
						textureInfo.imageView = colormap->view;
						writes[0].dstSet = colormapResources.descriptorSets[descriptorSetIndex];
						vkUpdateDescriptorSets(appState.Device, 1, writes, 0, nullptr);
						colormapResources.bound[descriptorSetIndex] = colormap;
					}
					vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.aliasHoley.pipelineLayout, 1, 1, &colormapResources.descriptorSets[descriptorSetIndex], 0, nullptr);
					previousColormapDescriptorSet = colormapResources.descriptorSets[descriptorSetIndex];
					descriptorSetIndex++;
				}
				auto texture = loaded.texture.texture;
				if (previousTexture != texture)
				{
					vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.aliasHoley.pipelineLayout, 2, 1, &texture->descriptorSet, 0, nullptr);
					previousTexture = texture;
				}
				auto indices = loaded.indices.indices.buffer;
				if (previousIndices != indices)
				{
					vkCmdBindIndexBuffer(commandBuffer, indices->buffer, 0, loaded.indices.indices.indexType);
					previousIndices = indices;
				}
				vkCmdDrawIndexed(commandBuffer, loaded.count, 1, loaded.indices.indices.firstIndex, 0, 0);
			}
#if !defined(NDEBUG) || defined(ENABLE_DEBUG_UTILS)
			appState.vkCmdEndDebugUtilsLabelEXT(commandBuffer);
#endif
		}
		if (appState.Scene.aliasHoleyAlpha.last >= 0)
		{
#if !defined(NDEBUG) || defined(ENABLE_DEBUG_UTILS)
			renderLabel.pLabelName = ALIAS_HOLEY_ALPHA_NAME;
			appState.vkCmdBeginDebugUtilsLabelEXT(commandBuffer, &renderLabel);
#endif
			vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.aliasHoleyAlpha.pipeline);
			vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.aliasHoleyAlpha.pipelineLayout, 0, 1, &sceneMatricesAndPaletteResources.descriptorSet, 0, nullptr);
			pushConstants[3] = 0;
			pushConstants[7] = 0;
			pushConstants[11] = 0;
			pushConstants[15] = 1;
			SharedMemoryBuffer* previousVertices = nullptr;
			SharedMemoryBuffer* previousTexCoords = nullptr;
			VkDescriptorSet previousColormapDescriptorSet = VK_NULL_HANDLE;
			SharedMemoryTexture* previousTexture = nullptr;
			SharedMemoryBuffer* previousIndices = nullptr;
			for (auto i = 0; i <= appState.Scene.aliasHoleyAlpha.last; i++)
			{
				auto& loaded = appState.Scene.aliasHoleyAlpha.loaded[i];
				auto vertices = loaded.vertices.buffer;
				if (previousVertices != vertices)
				{
					vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertices->buffer, &appState.NoOffset);
					previousVertices = vertices;
				}
				auto texCoords = loaded.texCoords.buffer;
				if (previousTexCoords != texCoords)
				{
					vkCmdBindVertexBuffers(commandBuffer, 1, 1, &texCoords->buffer, &appState.NoOffset);
					previousTexCoords = texCoords;
				}
				VkDeviceSize attributeOffset = aliasAttributeBase + loaded.firstAttribute * sizeof(float);
				vkCmdBindVertexBuffers(commandBuffer, 2, 1, &attributes->buffer, &attributeOffset);
				SetPushConstants(loaded, pushConstants);
				vkCmdPushConstants(commandBuffer, appState.Scene.aliasHoleyAlpha.pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, 16 * sizeof(float), pushConstants);
				if (loaded.isHostColormap)
				{
					if (previousColormapDescriptorSet != host_colormapResources.descriptorSet)
					{
						vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.aliasHoleyAlpha.pipelineLayout, 1, 1, &host_colormapResources.descriptorSet, 0, nullptr);
						previousColormapDescriptorSet = host_colormapResources.descriptorSet;
					}
				}
				else
				{
					auto colormap = loaded.colormap.texture;
					if (colormapResources.bound[descriptorSetIndex] != colormap)
					{
						textureInfo.sampler = appState.Scene.sampler;
						textureInfo.imageView = colormap->view;
						writes[0].dstSet = colormapResources.descriptorSets[descriptorSetIndex];
						vkUpdateDescriptorSets(appState.Device, 1, writes, 0, nullptr);
						colormapResources.bound[descriptorSetIndex] = colormap;
					}
					vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.aliasHoleyAlpha.pipelineLayout, 1, 1, &colormapResources.descriptorSets[descriptorSetIndex], 0, nullptr);
					previousColormapDescriptorSet = colormapResources.descriptorSets[descriptorSetIndex];
					descriptorSetIndex++;
				}
				auto texture = loaded.texture.texture;
				if (previousTexture != texture)
				{
					vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.aliasHoleyAlpha.pipelineLayout, 2, 1, &texture->descriptorSet, 0, nullptr);
					previousTexture = texture;
				}
				auto indices = loaded.indices.indices.buffer;
				if (previousIndices != indices)
				{
					vkCmdBindIndexBuffer(commandBuffer, indices->buffer, 0, loaded.indices.indices.indexType);
					previousIndices = indices;
				}
				vkCmdDrawIndexed(commandBuffer, loaded.count, 1, loaded.indices.indices.firstIndex, 0, 0);
			}
#if !defined(NDEBUG) || defined(ENABLE_DEBUG_UTILS)
			appState.vkCmdEndDebugUtilsLabelEXT(commandBuffer);
#endif
		}
		if (appState.Scene.aliasAlphaColoredLights.last >= 0)
		{
#if !defined(NDEBUG) || defined(ENABLE_DEBUG_UTILS)
			renderLabel.pLabelName = ALIAS_ALPHA_COLORED_LIGHTS_NAME;
			appState.vkCmdBeginDebugUtilsLabelEXT(commandBuffer, &renderLabel);
#endif
			vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.aliasAlphaColoredLights.pipeline);
			vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.aliasAlphaColoredLights.pipelineLayout, 0, 1, &sceneMatricesAndNeutralPaletteResources.descriptorSet, 0, nullptr);
			pushConstants[3] = 0;
			pushConstants[7] = 0;
			pushConstants[11] = 0;
			pushConstants[15] = 1;
			SetTintPushConstants(pushConstants, 16);
			SharedMemoryBuffer* previousVertices = nullptr;
			SharedMemoryBuffer* previousTexCoords = nullptr;
			SharedMemoryTexture* previousTexture = nullptr;
			SharedMemoryBuffer* previousIndices = nullptr;
			for (auto i = 0; i <= appState.Scene.aliasAlphaColoredLights.last; i++)
			{
				auto& loaded = appState.Scene.aliasAlphaColoredLights.loaded[i];
				auto vertices = loaded.vertices.buffer;
				if (previousVertices != vertices)
				{
					vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertices->buffer, &appState.NoOffset);
					previousVertices = vertices;
				}
				auto texCoords = loaded.texCoords.buffer;
				if (previousTexCoords != texCoords)
				{
					vkCmdBindVertexBuffers(commandBuffer, 1, 1, &texCoords->buffer, &appState.NoOffset);
					previousTexCoords = texCoords;
				}
				VkDeviceSize attributeOffset = aliasAttributeBase + loaded.firstAttribute * sizeof(float);
				vkCmdBindVertexBuffers(commandBuffer, 2, 1, &attributes->buffer, &attributeOffset);
				SetPushConstants(loaded, pushConstants);
				vkCmdPushConstants(commandBuffer, appState.Scene.aliasAlphaColoredLights.pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, 21 * sizeof(float), pushConstants);
				auto texture = loaded.texture.texture;
				if (previousTexture != texture)
				{
					vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.aliasAlphaColoredLights.pipelineLayout, 1, 1, &texture->descriptorSet, 0, nullptr);
					previousTexture = texture;
				}
				auto indices = loaded.indices.indices.buffer;
				if (previousIndices != indices)
				{
					vkCmdBindIndexBuffer(commandBuffer, indices->buffer, 0, loaded.indices.indices.indexType);
					previousIndices = indices;
				}
				vkCmdDrawIndexed(commandBuffer, loaded.count, 1, loaded.indices.indices.firstIndex, 0, 0);
			}
#if !defined(NDEBUG) || defined(ENABLE_DEBUG_UTILS)
			appState.vkCmdEndDebugUtilsLabelEXT(commandBuffer);
#endif
		}
		if (appState.Scene.aliasHoleyColoredLights.last >= 0)
		{
#if !defined(NDEBUG) || defined(ENABLE_DEBUG_UTILS)
			renderLabel.pLabelName = ALIAS_HOLEY_COLORED_LIGHTS_NAME;
			appState.vkCmdBeginDebugUtilsLabelEXT(commandBuffer, &renderLabel);
#endif
			vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.aliasHoleyColoredLights.pipeline);
			vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.aliasHoleyColoredLights.pipelineLayout, 0, 1, &sceneMatricesAndNeutralPaletteResources.descriptorSet, 0, nullptr);
			pushConstants[3] = 0;
			pushConstants[7] = 0;
			pushConstants[11] = 0;
			pushConstants[15] = 1;
			SetTintPushConstants(pushConstants, 16);
			SharedMemoryBuffer* previousVertices = nullptr;
			SharedMemoryBuffer* previousTexCoords = nullptr;
			SharedMemoryTexture* previousTexture = nullptr;
			SharedMemoryBuffer* previousIndices = nullptr;
			for (auto i = 0; i <= appState.Scene.aliasHoleyColoredLights.last; i++)
			{
				auto& loaded = appState.Scene.aliasHoleyColoredLights.loaded[i];
				auto vertices = loaded.vertices.buffer;
				if (previousVertices != vertices)
				{
					vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertices->buffer, &appState.NoOffset);
					previousVertices = vertices;
				}
				auto texCoords = loaded.texCoords.buffer;
				if (previousTexCoords != texCoords)
				{
					vkCmdBindVertexBuffers(commandBuffer, 1, 1, &texCoords->buffer, &appState.NoOffset);
					previousTexCoords = texCoords;
				}
				VkDeviceSize attributeOffset = aliasAttributeBase + loaded.firstAttribute * sizeof(float);
				vkCmdBindVertexBuffers(commandBuffer, 2, 1, &attributes->buffer, &attributeOffset);
				SetPushConstants(loaded, pushConstants);
				vkCmdPushConstants(commandBuffer, appState.Scene.aliasHoleyColoredLights.pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, 21 * sizeof(float), pushConstants);
				auto texture = loaded.texture.texture;
				if (previousTexture != texture)
				{
					vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.aliasHoleyColoredLights.pipelineLayout, 1, 1, &texture->descriptorSet, 0, nullptr);
					previousTexture = texture;
				}
				auto indices = loaded.indices.indices.buffer;
				if (previousIndices != indices)
				{
					vkCmdBindIndexBuffer(commandBuffer, indices->buffer, 0, loaded.indices.indices.indexType);
					previousIndices = indices;
				}
				vkCmdDrawIndexed(commandBuffer, loaded.count, 1, loaded.indices.indices.firstIndex, 0, 0);
			}
#if !defined(NDEBUG) || defined(ENABLE_DEBUG_UTILS)
			appState.vkCmdEndDebugUtilsLabelEXT(commandBuffer);
#endif
		}
		if (appState.Scene.aliasHoleyAlphaColoredLights.last >= 0)
		{
#if !defined(NDEBUG) || defined(ENABLE_DEBUG_UTILS)
			renderLabel.pLabelName = ALIAS_HOLEY_ALPHA_COLORED_LIGHTS_NAME;
			appState.vkCmdBeginDebugUtilsLabelEXT(commandBuffer, &renderLabel);
#endif
			vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.aliasHoleyAlphaColoredLights.pipeline);
			vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.aliasHoleyAlphaColoredLights.pipelineLayout, 0, 1, &sceneMatricesAndNeutralPaletteResources.descriptorSet, 0, nullptr);
			pushConstants[3] = 0;
			pushConstants[7] = 0;
			pushConstants[11] = 0;
			pushConstants[15] = 1;
			SetTintPushConstants(pushConstants, 16);
			SharedMemoryBuffer* previousVertices = nullptr;
			SharedMemoryBuffer* previousTexCoords = nullptr;
			SharedMemoryTexture* previousTexture = nullptr;
			SharedMemoryBuffer* previousIndices = nullptr;
			for (auto i = 0; i <= appState.Scene.aliasHoleyAlphaColoredLights.last; i++)
			{
				auto& loaded = appState.Scene.aliasHoleyAlphaColoredLights.loaded[i];
				auto vertices = loaded.vertices.buffer;
				if (previousVertices != vertices)
				{
					vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertices->buffer, &appState.NoOffset);
					previousVertices = vertices;
				}
				auto texCoords = loaded.texCoords.buffer;
				if (previousTexCoords != texCoords)
				{
					vkCmdBindVertexBuffers(commandBuffer, 1, 1, &texCoords->buffer, &appState.NoOffset);
					previousTexCoords = texCoords;
				}
				VkDeviceSize attributeOffset = aliasAttributeBase + loaded.firstAttribute * sizeof(float);
				vkCmdBindVertexBuffers(commandBuffer, 2, 1, &attributes->buffer, &attributeOffset);
				SetPushConstants(loaded, pushConstants);
				vkCmdPushConstants(commandBuffer, appState.Scene.aliasHoleyAlphaColoredLights.pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, 21 * sizeof(float), pushConstants);
				auto texture = loaded.texture.texture;
				if (previousTexture != texture)
				{
					vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.aliasHoleyAlphaColoredLights.pipelineLayout, 1, 1, &texture->descriptorSet, 0, nullptr);
					previousTexture = texture;
				}
				auto indices = loaded.indices.indices.buffer;
				if (previousIndices != indices)
				{
					vkCmdBindIndexBuffer(commandBuffer, indices->buffer, 0, loaded.indices.indices.indexType);
					previousIndices = indices;
				}
				vkCmdDrawIndexed(commandBuffer, loaded.count, 1, loaded.indices.indices.firstIndex, 0, 0);
			}
#if !defined(NDEBUG) || defined(ENABLE_DEBUG_UTILS)
			appState.vkCmdEndDebugUtilsLabelEXT(commandBuffer);
#endif
		}
		if (appState.Scene.viewmodels.last >= 0)
		{
#if !defined(NDEBUG) || defined(ENABLE_DEBUG_UTILS)
			renderLabel.pLabelName = VIEWMODELS_NAME;
			appState.vkCmdBeginDebugUtilsLabelEXT(commandBuffer, &renderLabel);
#endif
			vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.viewmodels.pipeline);
			vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.viewmodels.pipelineLayout, 0, 1, &sceneMatricesAndPaletteResources.descriptorSet, 0, nullptr);
			pushConstants[3] = 0;
			pushConstants[7] = 0;
			pushConstants[11] = 0;
			pushConstants[15] = 1;
			SharedMemoryBuffer* previousVertices = nullptr;
			SharedMemoryBuffer* previousTexCoords = nullptr;
			VkDescriptorSet previousColormapDescriptorSet = VK_NULL_HANDLE;
			SharedMemoryTexture* previousTexture = nullptr;
			SharedMemoryBuffer* previousIndices = nullptr;
			for (auto i = 0; i <= appState.Scene.viewmodels.last; i++)
			{
				auto& loaded = appState.Scene.viewmodels.loaded[i];
				auto vertices = loaded.vertices.buffer;
				if (previousVertices != vertices)
				{
					vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertices->buffer, &appState.NoOffset);
					previousVertices = vertices;
				}
				auto texCoords = loaded.texCoords.buffer;
				if (previousTexCoords != texCoords)
				{
					vkCmdBindVertexBuffers(commandBuffer, 1, 1, &texCoords->buffer, &appState.NoOffset);
					previousTexCoords = texCoords;
				}
				VkDeviceSize attributeOffset = aliasAttributeBase + loaded.firstAttribute * sizeof(float);
				vkCmdBindVertexBuffers(commandBuffer, 2, 1, &attributes->buffer, &attributeOffset);
				SetPushConstants(loaded, pushConstants);
				vkCmdPushConstants(commandBuffer, appState.Scene.viewmodels.pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, 16 * sizeof(float), pushConstants);
				if (loaded.isHostColormap)
				{
					if (previousColormapDescriptorSet != host_colormapResources.descriptorSet)
					{
						vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.viewmodels.pipelineLayout, 1, 1, &host_colormapResources.descriptorSet, 0, nullptr);
						previousColormapDescriptorSet = host_colormapResources.descriptorSet;
					}
				}
				else
				{
					auto colormap = loaded.colormap.texture;
					if (colormapResources.bound[descriptorSetIndex] != colormap)
					{
						textureInfo.sampler = appState.Scene.sampler;
						textureInfo.imageView = colormap->view;
						writes[0].dstSet = colormapResources.descriptorSets[descriptorSetIndex];
						vkUpdateDescriptorSets(appState.Device, 1, writes, 0, nullptr);
						colormapResources.bound[descriptorSetIndex] = colormap;
					}
					vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.viewmodels.pipelineLayout, 1, 1, &colormapResources.descriptorSets[descriptorSetIndex], 0, nullptr);
					previousColormapDescriptorSet = colormapResources.descriptorSets[descriptorSetIndex];
					descriptorSetIndex++;
				}
				auto texture = loaded.texture.texture;
				if (previousTexture != texture)
				{
					vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.viewmodels.pipelineLayout, 2, 1, &texture->descriptorSet, 0, nullptr);
					previousTexture = texture;
				}
				auto indices = loaded.indices.indices.buffer;
				if (previousIndices != indices)
				{
					vkCmdBindIndexBuffer(commandBuffer, indices->buffer, 0, loaded.indices.indices.indexType);
					previousIndices = indices;
				}
				vkCmdDrawIndexed(commandBuffer, loaded.count, 1, loaded.indices.indices.firstIndex, 0, 0);
			}
#if !defined(NDEBUG) || defined(ENABLE_DEBUG_UTILS)
			appState.vkCmdEndDebugUtilsLabelEXT(commandBuffer);
#endif
		}
		if (appState.Scene.viewmodelsHoley.last >= 0)
		{
#if !defined(NDEBUG) || defined(ENABLE_DEBUG_UTILS)
			renderLabel.pLabelName = VIEWMODELS_HOLEY_NAME;
			appState.vkCmdBeginDebugUtilsLabelEXT(commandBuffer, &renderLabel);
#endif
			vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.viewmodelsHoley.pipeline);
			vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.viewmodelsHoley.pipelineLayout, 0, 1, &sceneMatricesAndPaletteResources.descriptorSet, 0, nullptr);
			pushConstants[3] = 0;
			pushConstants[7] = 0;
			pushConstants[11] = 0;
			pushConstants[15] = 1;
			SharedMemoryBuffer* previousVertices = nullptr;
			SharedMemoryBuffer* previousTexCoords = nullptr;
			VkDescriptorSet previousColormapDescriptorSet = VK_NULL_HANDLE;
			SharedMemoryTexture* previousTexture = nullptr;
			SharedMemoryBuffer* previousIndices = nullptr;
			for (auto i = 0; i <= appState.Scene.viewmodelsHoley.last; i++)
			{
				auto& loaded = appState.Scene.viewmodelsHoley.loaded[i];
				auto vertices = loaded.vertices.buffer;
				if (previousVertices != vertices)
				{
					vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertices->buffer, &appState.NoOffset);
					previousVertices = vertices;
				}
				auto texCoords = loaded.texCoords.buffer;
				if (previousTexCoords != texCoords)
				{
					vkCmdBindVertexBuffers(commandBuffer, 1, 1, &texCoords->buffer, &appState.NoOffset);
					previousTexCoords = texCoords;
				}
				VkDeviceSize attributeOffset = aliasAttributeBase + loaded.firstAttribute * sizeof(float);
				vkCmdBindVertexBuffers(commandBuffer, 2, 1, &attributes->buffer, &attributeOffset);
				SetPushConstants(loaded, pushConstants);
				vkCmdPushConstants(commandBuffer, appState.Scene.viewmodelsHoley.pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, 16 * sizeof(float), pushConstants);
				if (loaded.isHostColormap)
				{
					if (previousColormapDescriptorSet != host_colormapResources.descriptorSet)
					{
						vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.viewmodelsHoley.pipelineLayout, 1, 1, &host_colormapResources.descriptorSet, 0, nullptr);
						previousColormapDescriptorSet = host_colormapResources.descriptorSet;
					}
				}
				else
				{
					auto colormap = loaded.colormap.texture;
					if (colormapResources.bound[descriptorSetIndex] != colormap)
					{
						textureInfo.sampler = appState.Scene.sampler;
						textureInfo.imageView = colormap->view;
						writes[0].dstSet = colormapResources.descriptorSets[descriptorSetIndex];
						vkUpdateDescriptorSets(appState.Device, 1, writes, 0, nullptr);
						colormapResources.bound[descriptorSetIndex] = colormap;
					}
					vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.viewmodelsHoley.pipelineLayout, 1, 1, &colormapResources.descriptorSets[descriptorSetIndex], 0, nullptr);
					previousColormapDescriptorSet = colormapResources.descriptorSets[descriptorSetIndex];
					descriptorSetIndex++;
				}
				auto texture = loaded.texture.texture;
				if (previousTexture != texture)
				{
					vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.viewmodelsHoley.pipelineLayout, 2, 1, &texture->descriptorSet, 0, nullptr);
					previousTexture = texture;
				}
				auto indices = loaded.indices.indices.buffer;
				if (previousIndices != indices)
				{
					vkCmdBindIndexBuffer(commandBuffer, indices->buffer, 0, loaded.indices.indices.indexType);
					previousIndices = indices;
				}
				vkCmdDrawIndexed(commandBuffer, loaded.count, 1, loaded.indices.indices.firstIndex, 0, 0);
			}
#if !defined(NDEBUG) || defined(ENABLE_DEBUG_UTILS)
			appState.vkCmdEndDebugUtilsLabelEXT(commandBuffer);
#endif
		}
		if (appState.Scene.viewmodelsColoredLights.last >= 0)
		{
#if !defined(NDEBUG) || defined(ENABLE_DEBUG_UTILS)
			renderLabel.pLabelName = VIEWMODELS_COLORED_LIGHTS_NAME;
			appState.vkCmdBeginDebugUtilsLabelEXT(commandBuffer, &renderLabel);
#endif
			vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.viewmodelsColoredLights.pipeline);
			vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.viewmodelsColoredLights.pipelineLayout, 0, 1, &sceneMatricesAndNeutralPaletteResources.descriptorSet, 0, nullptr);
			pushConstants[3] = 0;
			pushConstants[7] = 0;
			pushConstants[11] = 0;
			pushConstants[15] = 1;
			SetTintPushConstants(pushConstants, 16);
			SharedMemoryBuffer* previousVertices = nullptr;
			SharedMemoryBuffer* previousTexCoords = nullptr;
			SharedMemoryTexture* previousTexture = nullptr;
			SharedMemoryBuffer* previousIndices = nullptr;
			for (auto i = 0; i <= appState.Scene.viewmodelsColoredLights.last; i++)
			{
				auto& loaded = appState.Scene.viewmodelsColoredLights.loaded[i];
				auto vertices = loaded.vertices.buffer;
				if (previousVertices != vertices)
				{
					vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertices->buffer, &appState.NoOffset);
					previousVertices = vertices;
				}
				auto texCoords = loaded.texCoords.buffer;
				if (previousTexCoords != texCoords)
				{
					vkCmdBindVertexBuffers(commandBuffer, 1, 1, &texCoords->buffer, &appState.NoOffset);
					previousTexCoords = texCoords;
				}
				VkDeviceSize attributeOffset = aliasAttributeBase + loaded.firstAttribute * sizeof(float);
				vkCmdBindVertexBuffers(commandBuffer, 2, 1, &attributes->buffer, &attributeOffset);
				SetPushConstants(loaded, pushConstants);
				vkCmdPushConstants(commandBuffer, appState.Scene.viewmodelsColoredLights.pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, 21 * sizeof(float), pushConstants);
				auto texture = loaded.texture.texture;
				if (previousTexture != texture)
				{
					vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.viewmodelsColoredLights.pipelineLayout, 1, 1, &texture->descriptorSet, 0, nullptr);
					previousTexture = texture;
				}
				auto indices = loaded.indices.indices.buffer;
				if (previousIndices != indices)
				{
					vkCmdBindIndexBuffer(commandBuffer, indices->buffer, 0, loaded.indices.indices.indexType);
					previousIndices = indices;
				}
				vkCmdDrawIndexed(commandBuffer, loaded.count, 1, loaded.indices.indices.firstIndex, 0, 0);
			}
#if !defined(NDEBUG) || defined(ENABLE_DEBUG_UTILS)
			appState.vkCmdEndDebugUtilsLabelEXT(commandBuffer);
#endif
		}
		if (appState.Scene.viewmodelsHoleyColoredLights.last >= 0)
		{
#if !defined(NDEBUG) || defined(ENABLE_DEBUG_UTILS)
			renderLabel.pLabelName = VIEWMODELS_HOLEY_COLORED_LIGHTS_NAME;
			appState.vkCmdBeginDebugUtilsLabelEXT(commandBuffer, &renderLabel);
#endif
			vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.viewmodelsHoleyColoredLights.pipeline);
			vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.viewmodelsHoleyColoredLights.pipelineLayout, 0, 1, &sceneMatricesAndNeutralPaletteResources.descriptorSet, 0, nullptr);
			pushConstants[3] = 0;
			pushConstants[7] = 0;
			pushConstants[11] = 0;
			pushConstants[15] = 1;
			SetTintPushConstants(pushConstants, 16);
			SharedMemoryBuffer* previousVertices = nullptr;
			SharedMemoryBuffer* previousTexCoords = nullptr;
			SharedMemoryTexture* previousTexture = nullptr;
			SharedMemoryBuffer* previousIndices = nullptr;
			for (auto i = 0; i <= appState.Scene.viewmodelsHoleyColoredLights.last; i++)
			{
				auto& loaded = appState.Scene.viewmodelsHoleyColoredLights.loaded[i];
				auto vertices = loaded.vertices.buffer;
				if (previousVertices != vertices)
				{
					vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertices->buffer, &appState.NoOffset);
					previousVertices = vertices;
				}
				auto texCoords = loaded.texCoords.buffer;
				if (previousTexCoords != texCoords)
				{
					vkCmdBindVertexBuffers(commandBuffer, 1, 1, &texCoords->buffer, &appState.NoOffset);
					previousTexCoords = texCoords;
				}
				VkDeviceSize attributeOffset = aliasAttributeBase + loaded.firstAttribute * sizeof(float);
				vkCmdBindVertexBuffers(commandBuffer, 2, 1, &attributes->buffer, &attributeOffset);
				SetPushConstants(loaded, pushConstants);
				vkCmdPushConstants(commandBuffer, appState.Scene.viewmodelsHoleyColoredLights.pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, 21 * sizeof(float), pushConstants);
				auto texture = loaded.texture.texture;
				if (previousTexture != texture)
				{
					vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.viewmodelsHoleyColoredLights.pipelineLayout, 1, 1, &texture->descriptorSet, 0, nullptr);
					previousTexture = texture;
				}
				auto indices = loaded.indices.indices.buffer;
				if (previousIndices != indices)
				{
					vkCmdBindIndexBuffer(commandBuffer, indices->buffer, 0, loaded.indices.indices.indexType);
					previousIndices = indices;
				}
				vkCmdDrawIndexed(commandBuffer, loaded.count, 1, loaded.indices.indices.firstIndex, 0, 0);
			}
#if !defined(NDEBUG) || defined(ENABLE_DEBUG_UTILS)
			appState.vkCmdEndDebugUtilsLabelEXT(commandBuffer);
#endif
		}
	}
	else
	{
#if !defined(NDEBUG) || defined(ENABLE_DEBUG_UTILS)
		renderLabel.pLabelName = FLOOR_NAME;
		appState.vkCmdBeginDebugUtilsLabelEXT(commandBuffer, &renderLabel);
#endif
		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.floor.pipeline);
		vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertices->buffer, &appState.NoOffset);
		vkCmdBindVertexBuffers(commandBuffer, 1, 1, &attributes->buffer, &appState.NoOffset);
		if (!floorResources.created)
		{
			poolSizes[0].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			poolSizes[0].descriptorCount = 1;
			descriptorPoolCreateInfo.poolSizeCount = 1;
			CHECK_VKCMD(vkCreateDescriptorPool(appState.Device, &descriptorPoolCreateInfo, nullptr, &floorResources.descriptorPool));
			descriptorSetAllocateInfo.descriptorPool = floorResources.descriptorPool;
			descriptorSetAllocateInfo.descriptorSetCount = 1;
			descriptorSetAllocateInfo.pSetLayouts = &appState.Scene.singleImageLayout;
			CHECK_VKCMD(vkAllocateDescriptorSets(appState.Device, &descriptorSetAllocateInfo, &floorResources.descriptorSet));
			textureInfo.sampler = appState.Scene.sampler;
			textureInfo.imageView = appState.Scene.floorTexture.view;
			writes[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			writes[0].pImageInfo = &textureInfo;
			writes[0].dstSet = floorResources.descriptorSet;
			vkUpdateDescriptorSets(appState.Device, 1, writes, 0, nullptr);
			floorResources.created = true;
		}
		VkDescriptorSet descriptorSets[2];
		descriptorSets[0] = sceneMatricesResources.descriptorSet;
		descriptorSets[1] = floorResources.descriptorSet;
		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.floor.pipelineLayout, 0, 2, descriptorSets, 0, nullptr);
		if (appState.IndexTypeUInt8Enabled)
		{
			vkCmdBindIndexBuffer(commandBuffer, indices8->buffer, 0, VK_INDEX_TYPE_UINT8_EXT);
		}
		else
		{
			vkCmdBindIndexBuffer(commandBuffer, indices16->buffer, 0, VK_INDEX_TYPE_UINT16);
		}
		vkCmdDrawIndexed(commandBuffer, 4, 1, 0, 0, 0);
#if !defined(NDEBUG) || defined(ENABLE_DEBUG_UTILS)
		appState.vkCmdEndDebugUtilsLabelEXT(commandBuffer);
#endif
	}
	if (appState.Scene.controllerVerticesSize > 0)
	{
#if !defined(NDEBUG) || defined(ENABLE_DEBUG_UTILS)
		renderLabel.pLabelName = CONTROLLERS_NAME;
		appState.vkCmdBeginDebugUtilsLabelEXT(commandBuffer, &renderLabel);
#endif
		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.controllers.pipeline);
		vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertices->buffer, &controllerVertexBase);
		vkCmdBindVertexBuffers(commandBuffer, 1, 1, &attributes->buffer, &controllerAttributeBase);
		if (!controllerResources.created)
		{
			poolSizes[0].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			poolSizes[0].descriptorCount = 1;
			descriptorPoolCreateInfo.poolSizeCount = 1;
			CHECK_VKCMD(vkCreateDescriptorPool(appState.Device, &descriptorPoolCreateInfo, nullptr, &controllerResources.descriptorPool));
			descriptorSetAllocateInfo.descriptorPool = controllerResources.descriptorPool;
			descriptorSetAllocateInfo.descriptorSetCount = 1;
			descriptorSetAllocateInfo.pSetLayouts = &appState.Scene.singleImageLayout;
			CHECK_VKCMD(vkAllocateDescriptorSets(appState.Device, &descriptorSetAllocateInfo, &controllerResources.descriptorSet));
			textureInfo.sampler = appState.Scene.sampler;
			textureInfo.imageView = appState.Scene.controllerTexture.view;
			writes[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			writes[0].pImageInfo = &textureInfo;
			writes[0].dstSet = controllerResources.descriptorSet;
			vkUpdateDescriptorSets(appState.Device, 1, writes, 0, nullptr);
			controllerResources.created = true;
		}
		VkDescriptorSet descriptorSets[2];
		descriptorSets[0] = sceneMatricesResources.descriptorSet;
		descriptorSets[1] = controllerResources.descriptorSet;
		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.controllers.pipelineLayout, 0, 2, descriptorSets, 0, nullptr);
		if (appState.IndexTypeUInt8Enabled)
		{
			vkCmdBindIndexBuffer(commandBuffer, indices8->buffer, controllerIndexBase, VK_INDEX_TYPE_UINT8_EXT);
		}
		else
		{
			vkCmdBindIndexBuffer(commandBuffer, indices16->buffer, controllerIndexBase, VK_INDEX_TYPE_UINT16);
		}
		VkDeviceSize size = 0;
		if (appState.LeftController.PoseIsValid)
		{
			size += 2 * 36;
		}
		if (appState.RightController.PoseIsValid)
		{
			size += 2 * 36;
		}
		vkCmdDrawIndexed(commandBuffer, size, 1, 0, 0, 0);
#if !defined(NDEBUG) || defined(ENABLE_DEBUG_UTILS)
		appState.vkCmdEndDebugUtilsLabelEXT(commandBuffer);
#endif
	}
#if !defined(NDEBUG) || defined(ENABLE_DEBUG_UTILS)
	appState.vkCmdEndDebugUtilsLabelEXT(commandBuffer);
#endif
}

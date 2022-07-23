#include "AppState.h"
#include "PerFrame.h" // This header is specified second in the list to allow headers in AppState.h to include the core engine structs
#include "Utils.h"

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

byte PerFrame::AveragePixels(std::vector<byte>& pixdata)
{
	int        r,g,b;
	int        i;
	int        vis;
	int        pix;
	int        dr, dg, db;
	int        bestdistortion, distortion;
	int        bestcolor;
	byte    *pal;
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
		else if (pix >= 240)
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
	e = 240;

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
	byte* source = nullptr;
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
	std::vector<byte> pixdata;
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
	while (offset % 4 != 0)
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
	auto loadedBuffer = appState.Scene.buffers.firstAliasVertices;
	while (loadedBuffer != nullptr)
	{
		auto target = (byte*)(((unsigned char*)stagingBuffer->mapped) + offset);
		auto source = (trivertx_t*)loadedBuffer->source;
		for (auto i = 0; i < loadedBuffer->size; i += 2 * 4 * sizeof(byte))
		{
			auto x = source->v[0];
			auto y = source->v[1];
			auto z = source->v[2];
			*target++ = x;
			*target++ = y;
			*target++ = z;
			*target++ = 1;
			*target++ = x;
			*target++ = y;
			*target++ = z;
			*target++ = 1;
			source++;
		}
		offset += loadedBuffer->size;
		loadedBuffer = loadedBuffer->next;
	}
	while (offset % 4 != 0)
	{
		offset++;
	}
	auto loadedTexCoordsBuffer = appState.Scene.buffers.firstAliasTexCoords;
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
			*target++ = s + 0.5;
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
		texturedVertexBase = controllerVertexBase + appState.Scene.controllerVerticesSize;
		memcpy(((unsigned char*)stagingBuffer->mapped) + offset, d_lists.textured_vertices.data(), appState.Scene.texturedVerticesSize);
		offset += appState.Scene.texturedVerticesSize;
		particlePositionBase = texturedVertexBase + appState.Scene.texturedVerticesSize;
		memcpy(((unsigned char*)stagingBuffer->mapped) + offset, d_lists.particle_positions.data(), appState.Scene.particlePositionsSize);
		offset += appState.Scene.particlePositionsSize;
		coloredVertexBase = particlePositionBase + appState.Scene.particlePositionsSize;
		memcpy(((unsigned char*)stagingBuffer->mapped) + offset, d_lists.colored_vertices.data(), appState.Scene.coloredVerticesSize);
		offset += appState.Scene.coloredVerticesSize;
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
	texturedAttributeBase = controllerAttributeBase + appState.Scene.controllerAttributesSize;
	memcpy(((unsigned char*)stagingBuffer->mapped) + offset, d_lists.textured_attributes.data(), appState.Scene.texturedAttributesSize);
	offset += appState.Scene.texturedAttributesSize;
	colormappedAttributeBase = texturedAttributeBase + appState.Scene.texturedAttributesSize;
	memcpy(((unsigned char*)stagingBuffer->mapped) + offset, d_lists.colormapped_attributes.data(), appState.Scene.colormappedLightsSize);
	offset += appState.Scene.colormappedLightsSize;
	memcpy(((unsigned char*)stagingBuffer->mapped) + offset, d_lists.particle_colors.data(), appState.Scene.particleColorsSize);
	offset += appState.Scene.particleColorsSize;
	coloredColorBase = appState.Scene.particleColorsSize;
	memcpy(((unsigned char*)stagingBuffer->mapped) + offset, d_lists.colored_colors.data(), appState.Scene.coloredColorsSize);
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
		while (offset % 4 != 0)
		{
			offset++;
		}
	}
	memcpy(((unsigned char*)stagingBuffer->mapped) + offset, d_lists.colored_indices32.data(), appState.Scene.indices32Size);
	offset += appState.Scene.indices32Size;

	if (appState.Scene.sortedVerticesSize > 0)
	{
		SortedSurfaces::LoadVertices(appState.Scene.sorted.surfaces, appState.Scene.loadedSurfaces, stagingBuffer, offset);
		SortedSurfaces::LoadVertices(appState.Scene.sorted.surfacesRGBA, appState.Scene.loadedSurfacesRGBA, stagingBuffer, offset);
		SortedSurfaces::LoadVertices(appState.Scene.sorted.surfacesRGBANoGlow, appState.Scene.loadedSurfacesRGBANoGlow, stagingBuffer, offset);
		SortedSurfaces::LoadVertices(appState.Scene.sorted.surfacesRotated, appState.Scene.loadedSurfacesRotated, stagingBuffer, offset);
		SortedSurfaces::LoadVertices(appState.Scene.sorted.surfacesRotatedRGBA, appState.Scene.loadedSurfacesRotatedRGBA, stagingBuffer, offset);
		SortedSurfaces::LoadVertices(appState.Scene.sorted.surfacesRotatedRGBANoGlow, appState.Scene.loadedSurfacesRotatedRGBANoGlow, stagingBuffer, offset);
		SortedSurfaces::LoadVertices(appState.Scene.sorted.fences, appState.Scene.loadedFences, stagingBuffer, offset);
		SortedSurfaces::LoadVertices(appState.Scene.sorted.fencesRGBA, appState.Scene.loadedFencesRGBA, stagingBuffer, offset);
		SortedSurfaces::LoadVertices(appState.Scene.sorted.fencesRGBANoGlow, appState.Scene.loadedFencesRGBANoGlow, stagingBuffer, offset);
		SortedSurfaces::LoadVertices(appState.Scene.sorted.fencesRotated, appState.Scene.loadedFencesRotated, stagingBuffer, offset);
		SortedSurfaces::LoadVertices(appState.Scene.sorted.fencesRotatedRGBA, appState.Scene.loadedFencesRotatedRGBA, stagingBuffer, offset);
		SortedSurfaces::LoadVertices(appState.Scene.sorted.fencesRotatedRGBANoGlow, appState.Scene.loadedFencesRotatedRGBANoGlow, stagingBuffer, offset);
		SortedSurfaces::LoadVertices(appState.Scene.sorted.turbulent, appState.Scene.loadedTurbulent, stagingBuffer, offset);
		SortedSurfaces::LoadVertices(appState.Scene.sorted.turbulentRGBA, appState.Scene.loadedTurbulentRGBA, stagingBuffer, offset);
		SortedSurfaces::LoadVertices(appState.Scene.sorted.turbulentLit, appState.Scene.loadedTurbulentLit, stagingBuffer, offset);
		SortedSurfaces::LoadVertices(appState.Scene.sorted.turbulentLitRGBA, appState.Scene.loadedTurbulentLitRGBA, stagingBuffer, offset);
	}
	if (appState.Scene.sortedAttributesSize > 0)
	{
		SortedSurfaces::LoadAttributes(appState.Scene.sorted.surfaces, appState.Scene.loadedSurfaces, stagingBuffer, offset);
		SortedSurfaces::LoadAttributes(appState.Scene.sorted.surfacesRGBA, appState.Scene.loadedSurfacesRGBA, stagingBuffer, offset);
		SortedSurfaces::LoadAttributes(appState.Scene.sorted.surfacesRGBANoGlow, appState.Scene.loadedSurfacesRGBANoGlow, stagingBuffer, offset);
		SortedSurfaces::LoadAttributes(appState.Scene.sorted.surfacesRotated, appState.Scene.loadedSurfacesRotated, stagingBuffer, offset);
		SortedSurfaces::LoadAttributes(appState.Scene.sorted.surfacesRotatedRGBA, appState.Scene.loadedSurfacesRotatedRGBA, stagingBuffer, offset);
		SortedSurfaces::LoadAttributes(appState.Scene.sorted.surfacesRotatedRGBANoGlow, appState.Scene.loadedSurfacesRotatedRGBANoGlow, stagingBuffer, offset);
		SortedSurfaces::LoadAttributes(appState.Scene.sorted.fences, appState.Scene.loadedFences, stagingBuffer, offset);
		SortedSurfaces::LoadAttributes(appState.Scene.sorted.fencesRGBA, appState.Scene.loadedFencesRGBA, stagingBuffer, offset);
		SortedSurfaces::LoadAttributes(appState.Scene.sorted.fencesRGBANoGlow, appState.Scene.loadedFencesRGBANoGlow, stagingBuffer, offset);
		SortedSurfaces::LoadAttributes(appState.Scene.sorted.fencesRotated, appState.Scene.loadedFencesRotated, stagingBuffer, offset);
		SortedSurfaces::LoadAttributes(appState.Scene.sorted.fencesRotatedRGBA, appState.Scene.loadedFencesRotatedRGBA, stagingBuffer, offset);
		SortedSurfaces::LoadAttributes(appState.Scene.sorted.fencesRotatedRGBANoGlow, appState.Scene.loadedFencesRotatedRGBANoGlow, stagingBuffer, offset);
		SortedSurfaces::LoadAttributes(appState.Scene.sorted.turbulent, appState.Scene.loadedTurbulent, stagingBuffer, offset);
		SortedSurfaces::LoadAttributes(appState.Scene.sorted.turbulentRGBA, appState.Scene.loadedTurbulentRGBA, stagingBuffer, offset);
		SortedSurfaces::LoadAttributes(appState.Scene.sorted.turbulentLit, appState.Scene.loadedTurbulentLit, stagingBuffer, offset);
		SortedSurfaces::LoadAttributes(appState.Scene.sorted.turbulentLitRGBA, appState.Scene.loadedTurbulentLitRGBA, stagingBuffer, offset);
	}
	if (appState.Scene.sortedIndicesCount > 0)
	{
		if (appState.Scene.sortedIndices16Size > 0)
		{
			SortedSurfaces::LoadIndices16(appState.Scene.sorted.surfaces, appState.Scene.loadedSurfaces, stagingBuffer, offset);
			SortedSurfaces::LoadIndices16(appState.Scene.sorted.surfacesRGBA, appState.Scene.loadedSurfacesRGBA, stagingBuffer, offset);
			SortedSurfaces::LoadIndices16(appState.Scene.sorted.surfacesRGBANoGlow, appState.Scene.loadedSurfacesRGBANoGlow, stagingBuffer, offset);
			SortedSurfaces::LoadIndices16(appState.Scene.sorted.surfacesRotated, appState.Scene.loadedSurfacesRotated, stagingBuffer, offset);
			SortedSurfaces::LoadIndices16(appState.Scene.sorted.surfacesRotatedRGBA, appState.Scene.loadedSurfacesRotatedRGBA, stagingBuffer, offset);
			SortedSurfaces::LoadIndices16(appState.Scene.sorted.surfacesRotatedRGBANoGlow, appState.Scene.loadedSurfacesRotatedRGBANoGlow, stagingBuffer, offset);
			SortedSurfaces::LoadIndices16(appState.Scene.sorted.fences, appState.Scene.loadedFences, stagingBuffer, offset);
			SortedSurfaces::LoadIndices16(appState.Scene.sorted.fencesRGBA, appState.Scene.loadedFencesRGBA, stagingBuffer, offset);
			SortedSurfaces::LoadIndices16(appState.Scene.sorted.fencesRGBANoGlow, appState.Scene.loadedFencesRGBANoGlow, stagingBuffer, offset);
			SortedSurfaces::LoadIndices16(appState.Scene.sorted.fencesRotated, appState.Scene.loadedFencesRotated, stagingBuffer, offset);
			SortedSurfaces::LoadIndices16(appState.Scene.sorted.fencesRotatedRGBA, appState.Scene.loadedFencesRotatedRGBA, stagingBuffer, offset);
			SortedSurfaces::LoadIndices16(appState.Scene.sorted.fencesRotatedRGBANoGlow, appState.Scene.loadedFencesRotatedRGBANoGlow, stagingBuffer, offset);
			SortedSurfaces::LoadIndices16(appState.Scene.sorted.turbulent, appState.Scene.loadedTurbulent, stagingBuffer, offset);
			SortedSurfaces::LoadIndices16(appState.Scene.sorted.turbulentRGBA, appState.Scene.loadedTurbulentRGBA, stagingBuffer, offset);
			SortedSurfaces::LoadIndices16(appState.Scene.sorted.turbulentLit, appState.Scene.loadedTurbulentLit, stagingBuffer, offset);
			SortedSurfaces::LoadIndices16(appState.Scene.sorted.turbulentLitRGBA, appState.Scene.loadedTurbulentLitRGBA, stagingBuffer, offset);
			while (offset % 4 != 0)
			{
				offset++;
			}
		}
		else
		{
			SortedSurfaces::LoadIndices32(appState.Scene.sorted.surfaces, appState.Scene.loadedSurfaces, stagingBuffer, offset);
			SortedSurfaces::LoadIndices32(appState.Scene.sorted.surfacesRGBA, appState.Scene.loadedSurfacesRGBA, stagingBuffer, offset);
			SortedSurfaces::LoadIndices32(appState.Scene.sorted.surfacesRGBANoGlow, appState.Scene.loadedSurfacesRGBANoGlow, stagingBuffer, offset);
			SortedSurfaces::LoadIndices32(appState.Scene.sorted.surfacesRotated, appState.Scene.loadedSurfacesRotated, stagingBuffer, offset);
			SortedSurfaces::LoadIndices32(appState.Scene.sorted.surfacesRotatedRGBA, appState.Scene.loadedSurfacesRotatedRGBA, stagingBuffer, offset);
			SortedSurfaces::LoadIndices32(appState.Scene.sorted.surfacesRotatedRGBANoGlow, appState.Scene.loadedSurfacesRotatedRGBANoGlow, stagingBuffer, offset);
			SortedSurfaces::LoadIndices32(appState.Scene.sorted.fences, appState.Scene.loadedFences, stagingBuffer, offset);
			SortedSurfaces::LoadIndices32(appState.Scene.sorted.fencesRGBA, appState.Scene.loadedFencesRGBA, stagingBuffer, offset);
			SortedSurfaces::LoadIndices32(appState.Scene.sorted.fencesRGBANoGlow, appState.Scene.loadedFencesRGBANoGlow, stagingBuffer, offset);
			SortedSurfaces::LoadIndices32(appState.Scene.sorted.fencesRotated, appState.Scene.loadedFencesRotated, stagingBuffer, offset);
			SortedSurfaces::LoadIndices32(appState.Scene.sorted.fencesRotatedRGBA, appState.Scene.loadedFencesRotatedRGBA, stagingBuffer, offset);
			SortedSurfaces::LoadIndices32(appState.Scene.sorted.fencesRotatedRGBANoGlow, appState.Scene.loadedFencesRotatedRGBANoGlow, stagingBuffer, offset);
			SortedSurfaces::LoadIndices32(appState.Scene.sorted.turbulent, appState.Scene.loadedTurbulent, stagingBuffer, offset);
			SortedSurfaces::LoadIndices32(appState.Scene.sorted.turbulentRGBA, appState.Scene.loadedTurbulentRGBA, stagingBuffer, offset);
			SortedSurfaces::LoadIndices32(appState.Scene.sorted.turbulentLit, appState.Scene.loadedTurbulentLit, stagingBuffer, offset);
			SortedSurfaces::LoadIndices32(appState.Scene.sorted.turbulentLitRGBA, appState.Scene.loadedTurbulentLitRGBA, stagingBuffer, offset);
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
	}
	if (appState.Scene.host_colormapSize > 0)
	{
		memcpy(((unsigned char*)stagingBuffer->mapped) + offset, ::host_colormap.data(), appState.Scene.host_colormapSize);
		offset += appState.Scene.host_colormapSize;
	}
	auto loadedLightmap = appState.Scene.lightmaps.first;
	while (loadedLightmap != nullptr)
	{
		auto source = loadedLightmap->source;
		auto target = (float*)(((unsigned char*)stagingBuffer->mapped) + offset);
		for (auto i = 0; i < loadedLightmap->size; i++)
		{
			*target++ = (float)(*source++);
		}
		offset += loadedLightmap->size;
		loadedLightmap = loadedLightmap->next;
	}
	for (auto& entry : appState.Scene.surfaceTextures)
	{
		auto loadedTexture = entry.second.first;
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
	for (auto i = 0; i <= appState.Scene.lastAlias; i++)
	{
		auto& alias = d_lists.alias[i];
		if (!alias.is_host_colormap)
		{
			memcpy(((unsigned char*)stagingBuffer->mapped) + offset, alias.colormap.data(), 16384);
			offset += 16384;
		}
	}
	for (auto i = 0; i <= appState.Scene.lastViewmodel; i++)
	{
		auto& viewmodel = d_lists.viewmodels[i];
		if (!viewmodel.is_host_colormap)
		{
			memcpy(((unsigned char*)stagingBuffer->mapped) + offset, viewmodel.colormap.data(), 16384);
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
		auto loadedTexture = entry.second.first;
		while (loadedTexture != nullptr)
		{
			memcpy(((unsigned char*)stagingBuffer->mapped) + offset, loadedTexture->source, loadedTexture->size);
			offset += loadedTexture->allocated;
			loadedTexture = loadedTexture->next;
		}
	}
}

void PerFrame::FillColormapTextures(AppState& appState, LoadedAlias& loaded)
{
	if (!loaded.isHostColormap)
	{
		Texture* texture;
		if (colormaps.oldTextures != nullptr)
		{
			texture = colormaps.oldTextures;
			colormaps.oldTextures = texture->next;
		}
		else
		{
			texture = new Texture();
			texture->Create(appState, 256, 64, VK_FORMAT_R8_UINT, 1, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
		}
		texture->Fill(appState, appState.Scene.stagingBuffer);
		appState.Scene.stagingBuffer.offset += 16384;
		colormaps.MoveToFront(texture);
		loaded.colormapped.colormap.texture = texture;
		colormapCount++;
	}
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
			appState.Scene.AddToBufferBarrier(loaded->indices.buffer->buffer);
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

void PerFrame::FillFromStagingBuffer(AppState& appState, Buffer* stagingBuffer, LoadedSharedMemoryBuffer* first, VkBufferCopy& bufferCopy) const
{
	auto loaded = first;
	while (loaded != nullptr)
	{
		bufferCopy.size = loaded->size;
		vkCmdCopyBuffer(commandBuffer, stagingBuffer->buffer, loaded->buffer->buffer, 1, &bufferCopy);
		bufferCopy.srcOffset += loaded->size;
		appState.Scene.AddToBufferBarrier(loaded->buffer->buffer);
		loaded = loaded->next;
	}
}

void PerFrame::FillFromStagingBuffer(AppState& appState, Buffer* stagingBuffer)
{
	appState.Scene.stagingBuffer.lastBarrier = -1;
	appState.Scene.stagingBuffer.descriptorSetsInUse.clear();

	VkBufferCopy bufferCopy { };

	SharedMemoryBuffer* previousBuffer = nullptr;
	FillAliasFromStagingBuffer(appState, stagingBuffer, appState.Scene.indexBuffers.firstAliasIndices8, bufferCopy, previousBuffer);

	while (bufferCopy.srcOffset % 4 != 0)
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

	FillFromStagingBuffer(appState, stagingBuffer, appState.Scene.buffers.firstAliasVertices, bufferCopy);

	while (bufferCopy.srcOffset % 4 != 0)
	{
		bufferCopy.srcOffset++;
	}

	auto loadedTexCoordsBuffer = appState.Scene.buffers.firstAliasTexCoords;
	while (loadedTexCoordsBuffer != nullptr)
	{
		bufferCopy.size = loadedTexCoordsBuffer->size;
		vkCmdCopyBuffer(commandBuffer, stagingBuffer->buffer, loadedTexCoordsBuffer->buffer->buffer, 1, &bufferCopy);
		bufferCopy.srcOffset += bufferCopy.size;
		appState.Scene.AddToBufferBarrier(loadedTexCoordsBuffer->buffer->buffer);
		loadedTexCoordsBuffer = loadedTexCoordsBuffer->next;
	}

	if (appState.Scene.verticesSize > 0)
	{
		bufferCopy.size = appState.Scene.verticesSize;
		vkCmdCopyBuffer(commandBuffer, stagingBuffer->buffer, vertices->buffer, 1, &bufferCopy);
		bufferCopy.srcOffset += bufferCopy.size;
		appState.Scene.AddToBufferBarrier(vertices->buffer);
	}
	if (appState.Scene.attributesSize > 0)
	{
		bufferCopy.size = appState.Scene.attributesSize;
		vkCmdCopyBuffer(commandBuffer, stagingBuffer->buffer, attributes->buffer, 1, &bufferCopy);
		bufferCopy.srcOffset += bufferCopy.size;
		appState.Scene.AddToBufferBarrier(attributes->buffer);
	}
	if (appState.Scene.colorsSize > 0)
	{
		bufferCopy.size = appState.Scene.colorsSize;
		vkCmdCopyBuffer(commandBuffer, stagingBuffer->buffer, colors->buffer, 1, &bufferCopy);
		bufferCopy.srcOffset += bufferCopy.size;
		appState.Scene.AddToBufferBarrier(colors->buffer);
	}

	if (appState.Scene.indices8Size > 0)
	{
		bufferCopy.size = appState.Scene.indices8Size;
		vkCmdCopyBuffer(commandBuffer, stagingBuffer->buffer, indices8->buffer, 1, &bufferCopy);
		bufferCopy.srcOffset += bufferCopy.size;
		appState.Scene.AddToBufferBarrier(indices8->buffer);

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
		appState.Scene.AddToBufferBarrier(indices16->buffer);

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
		appState.Scene.AddToBufferBarrier(indices32->buffer);
	}

	if (appState.Scene.sortedVerticesSize > 0)
	{
		bufferCopy.size = appState.Scene.sortedVerticesSize;
		bufferCopy.dstOffset = appState.Scene.verticesSize;
		vkCmdCopyBuffer(commandBuffer, stagingBuffer->buffer, vertices->buffer, 1, &bufferCopy);
		bufferCopy.dstOffset = 0;
		bufferCopy.srcOffset += bufferCopy.size;
		if (appState.Scene.verticesSize == 0)
		{
			appState.Scene.AddToBufferBarrier(vertices->buffer);
		}
	}
	if (appState.Scene.sortedAttributesSize > 0)
	{
		bufferCopy.size = appState.Scene.sortedAttributesSize;
		bufferCopy.dstOffset = appState.Scene.attributesSize;
		vkCmdCopyBuffer(commandBuffer, stagingBuffer->buffer, attributes->buffer, 1, &bufferCopy);
		bufferCopy.dstOffset = 0;
		bufferCopy.srcOffset += bufferCopy.size;
		if (appState.Scene.attributesSize == 0)
		{
			appState.Scene.AddToBufferBarrier(attributes->buffer);
		}
	}
	if (appState.Scene.sortedIndicesCount > 0)
	{
		if (appState.Scene.sortedIndices16Size > 0)
		{
			bufferCopy.size = appState.Scene.sortedIndices16Size;
			bufferCopy.dstOffset = appState.Scene.indices16Size;
			vkCmdCopyBuffer(commandBuffer, stagingBuffer->buffer, indices16->buffer, 1, &bufferCopy);
			bufferCopy.dstOffset = 0;
			bufferCopy.srcOffset += bufferCopy.size;
			if (appState.Scene.indices16Size == 0)
			{
				appState.Scene.AddToBufferBarrier(indices16->buffer);
			}
			while (bufferCopy.srcOffset % 4 != 0)
			{
				bufferCopy.srcOffset++;
			}
		}
		else
		{
			bufferCopy.size = appState.Scene.sortedIndices32Size;
			bufferCopy.dstOffset = appState.Scene.indices32Size;
			vkCmdCopyBuffer(commandBuffer, stagingBuffer->buffer, indices32->buffer, 1, &bufferCopy);
			bufferCopy.dstOffset = 0;
			bufferCopy.srcOffset += bufferCopy.size;
			if (appState.Scene.indices32Size == 0)
			{
				appState.Scene.AddToBufferBarrier(indices32->buffer);
			}
		}
	}

	if (appState.Scene.paletteSize > 0)
	{
		bufferCopy.size = appState.Scene.paletteSize;
		vkCmdCopyBuffer(commandBuffer, stagingBuffer->buffer, palette->buffer, 1, &bufferCopy);
		bufferCopy.srcOffset += bufferCopy.size;
		appState.Scene.AddToBufferBarrier(palette->buffer);
	}

	if (appState.Scene.stagingBuffer.lastBarrier >= 0)
	{
		vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, 0, 0, nullptr, appState.Scene.stagingBuffer.lastBarrier + 1, appState.Scene.stagingBuffer.bufferBarriers.data(), 0, nullptr);
	}

	appState.Scene.stagingBuffer.buffer = stagingBuffer;
	appState.Scene.stagingBuffer.offset = bufferCopy.srcOffset;
	appState.Scene.stagingBuffer.commandBuffer = commandBuffer;
	appState.Scene.stagingBuffer.lastBarrier = -1;

	if (appState.Scene.host_colormapSize > 0)
	{
		host_colormap->Fill(appState, appState.Scene.stagingBuffer);
		appState.Scene.stagingBuffer.offset += appState.Scene.host_colormapSize;
	}
	auto loadedLightmap = appState.Scene.lightmaps.first;
	while (loadedLightmap != nullptr)
	{
		loadedLightmap->lightmap->Fill(appState, appState.Scene.stagingBuffer);
		appState.Scene.stagingBuffer.offset += loadedLightmap->size;
		loadedLightmap = loadedLightmap->next;
	}
	for (auto& entry : appState.Scene.surfaceTextures)
	{
		auto loadedTexture = entry.second.first;
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

	for (auto i = 0; i <= appState.Scene.lastAlias; i++)
	{
		FillColormapTextures(appState, appState.Scene.loadedAlias[i]);
	}
	for (auto i = 0; i <= appState.Scene.lastViewmodel; i++)
	{
		FillColormapTextures(appState, appState.Scene.loadedViewmodels[i]);
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
		auto loadedTexture = entry.second.first;
		while (loadedTexture != nullptr)
		{
			loadedTexture->texture->FillMipmapped(appState, appState.Scene.stagingBuffer, loadedTexture->mips, loadedTexture->index);
			appState.Scene.stagingBuffer.offset += loadedTexture->allocated;
			loadedTexture = loadedTexture->next;
		}
	}

	if (appState.Scene.stagingBuffer.lastBarrier >= 0)
	{
		vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, appState.Scene.stagingBuffer.lastBarrier + 1, appState.Scene.stagingBuffer.imageBarriers.data());
	}
}

void PerFrame::Reset(AppState& appState)
{
	cachedVertices.Reset(appState);
	cachedAttributes.Reset(appState);
	cachedIndices8.Reset(appState);
	cachedIndices16.Reset(appState);
	cachedIndices32.Reset(appState);
	cachedColors.Reset(appState);
	stagingBuffers.Reset(appState);
	colormaps.Reset(appState);
	colormapCount = 0;
	vertices = nullptr;
	attributes = nullptr;
	indices8 = nullptr;
	indices16 = nullptr;
	indices32 = nullptr;
	colors = nullptr;
}

void PerFrame::SetPushConstants(const LoadedAlias& loaded, float pushConstants[])
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

void PerFrame::Render(AppState& appState)
{
	if (appState.Scene.lastSurface < 0 &&
		appState.Scene.lastSurfaceRGBA < 0 &&
		appState.Scene.lastSurfaceRGBANoGlow < 0 &&
		appState.Scene.lastSurfaceRotated < 0 &&
		appState.Scene.lastSurfaceRotatedRGBA < 0 &&
		appState.Scene.lastSurfaceRotatedRGBANoGlow < 0 &&
		appState.Scene.lastFence < 0 &&
		appState.Scene.lastFenceRotated < 0 &&
		appState.Scene.lastTurbulent < 0 &&
		appState.Scene.lastTurbulentRGBA < 0 &&
		appState.Scene.lastTurbulentLit < 0 &&
		appState.Scene.lastTurbulentLitRGBA < 0 &&
		appState.Scene.lastAlias < 0 &&
		appState.Scene.lastViewmodel < 0 &&
		appState.Scene.lastSky < 0 &&
		appState.Scene.verticesSize == 0)
	{
		return;
	}

	VkDescriptorPoolSize poolSizes[3] { };

	VkDescriptorPoolCreateInfo descriptorPoolCreateInfo { VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO };
	descriptorPoolCreateInfo.maxSets = 1;
	descriptorPoolCreateInfo.pPoolSizes = poolSizes;

	VkDescriptorSetAllocateInfo descriptorSetAllocateInfo { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
	descriptorSetAllocateInfo.descriptorSetCount = 1;

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

	if (!host_colormapResources.created && host_colormap != nullptr)
	{
		poolSizes[0].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		poolSizes[0].descriptorCount = 1;
		textureInfo.sampler = appState.Scene.samplers[host_colormap->mipCount];
		textureInfo.imageView = host_colormap->view;
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
	if (!sceneMatricesResources.created || !sceneMatricesAndPaletteResources.created || (!sceneMatricesAndColormapResources.created && host_colormap != nullptr))
	{
		poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		poolSizes[0].descriptorCount = 1;
		VkDescriptorBufferInfo bufferInfo[2] { };
		bufferInfo[0].buffer = matrices.buffer;
		bufferInfo[0].range = matrices.size;
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
		if (!sceneMatricesAndPaletteResources.created || (!sceneMatricesAndColormapResources.created && host_colormap != nullptr))
		{
			poolSizes[1].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			poolSizes[1].descriptorCount = 1;
			bufferInfo[1].buffer = palette->buffer;
			bufferInfo[1].range = palette->size;
			writes[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			writes[1].pBufferInfo = bufferInfo + 1;
			if (!sceneMatricesAndPaletteResources.created)
			{
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
			if (!sceneMatricesAndColormapResources.created && host_colormap != nullptr)
			{
				poolSizes[2].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
				poolSizes[2].descriptorCount = 1;
				textureInfo.sampler = appState.Scene.samplers[host_colormap->mipCount];
				textureInfo.imageView = host_colormap->view;
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
	if (appState.Mode == AppWorldMode)
	{
		poolSizes[0].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		writes[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		writes[0].pImageInfo = &textureInfo;
		float pushConstants[20];
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
				textureInfo.sampler = appState.Scene.samplers[sky->mipCount];
				textureInfo.imageView = sky->view;
				writes[0].dstSet = skyResources.descriptorSet;
				vkUpdateDescriptorSets(appState.Device, 1, writes, 0, nullptr);
				skyResources.created = true;
			}
			vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.sky.pipeline);
			vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertices->buffer, &texturedVertexBase);
			vkCmdBindVertexBuffers(commandBuffer, 1, 1, &attributes->buffer, &texturedAttributeBase);
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
				textureInfo.sampler = appState.Scene.samplers[skyRGBA->mipCount];
				textureInfo.imageView = skyRGBA->view;
				writes[0].dstSet = skyRGBAResources.descriptorSet;
				vkUpdateDescriptorSets(appState.Device, 1, writes, 0, nullptr);
				skyRGBAResources.created = true;
			}
			vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.skyRGBA.pipeline);
			vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertices->buffer, &texturedVertexBase);
			vkCmdBindVertexBuffers(commandBuffer, 1, 1, &attributes->buffer, &texturedAttributeBase);
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
		}
		if (appState.Scene.lastSurface >= 0)
		{
			vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.surfaces.pipeline);
			vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.surfaces.pipelineLayout, 0, 1, &sceneMatricesAndColormapResources.descriptorSet, 0, nullptr);
			VkDeviceSize indexBase = 0;
			vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertices->buffer, &appState.Scene.verticesSize);
			vkCmdBindVertexBuffers(commandBuffer, 1, 1, &attributes->buffer, &appState.Scene.attributesSize);
			if (appState.Scene.sortedIndices16Size > 0)
			{
				vkCmdBindIndexBuffer(commandBuffer, indices16->buffer, appState.Scene.indices16Size, VK_INDEX_TYPE_UINT16);
			}
			else
			{
				vkCmdBindIndexBuffer(commandBuffer, indices32->buffer, appState.Scene.indices32Size, VK_INDEX_TYPE_UINT32);
			}
			for (auto& entry : appState.Scene.sorted.surfaces)
			{
				vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.surfaces.pipelineLayout, 1, 1, &entry.lightmap, 0, nullptr);
				for (auto& subEntry : entry.textures)
				{
					vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.surfaces.pipelineLayout, 2, 1, &subEntry.texture, 0, nullptr);
					vkCmdDrawIndexed(commandBuffer, subEntry.indexCount, 1, indexBase, 0, 0);
					indexBase += subEntry.indexCount;
				}
			}
		}
		if (appState.Scene.lastSurfaceRGBA >= 0)
		{
			vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.surfacesRGBA.pipeline);
			vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.surfacesRGBA.pipelineLayout, 0, 1, &sceneMatricesResources.descriptorSet, 0, nullptr);
			auto verticesBase = appState.Scene.verticesSize + appState.Scene.sortedSurfaceRGBAVerticesBase;
			auto attributesBase = appState.Scene.attributesSize + appState.Scene.sortedSurfaceRGBAAttributesBase;
			VkDeviceSize indexBase = 0;
			vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertices->buffer, &verticesBase);
			vkCmdBindVertexBuffers(commandBuffer, 1, 1, &attributes->buffer, &attributesBase);
			vkCmdBindVertexBuffers(commandBuffer, 2, 1, &attributes->buffer, &attributesBase);
			if (appState.Scene.sortedIndices16Size > 0)
			{
				vkCmdBindIndexBuffer(commandBuffer, indices16->buffer, appState.Scene.indices16Size + appState.Scene.sortedSurfaceRGBAIndicesBase, VK_INDEX_TYPE_UINT16);
			}
			else
			{
				vkCmdBindIndexBuffer(commandBuffer, indices32->buffer, appState.Scene.indices32Size + appState.Scene.sortedSurfaceRGBAIndicesBase, VK_INDEX_TYPE_UINT32);
			}
			pushConstants[0] = GammaCorrect(v_blend[0]);
			pushConstants[1] = GammaCorrect(v_blend[1]);
			pushConstants[2] = GammaCorrect(v_blend[2]);
			pushConstants[3] = GammaCorrect(v_blend[3]);
			vkCmdPushConstants(commandBuffer, appState.Scene.surfacesRGBA.pipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, 4 * sizeof(float), &pushConstants);
			for (auto& entry : appState.Scene.sorted.surfacesRGBA)
			{
				vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.surfacesRGBA.pipelineLayout, 1, 1, &entry.lightmap, 0, nullptr);
				for (auto& subEntry : entry.textures)
				{
					vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.surfacesRGBA.pipelineLayout, 2, 1, &subEntry.texture, 0, nullptr);
					vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.surfacesRGBA.pipelineLayout, 3, 1, &subEntry.glowTexture, 0, nullptr);
					vkCmdDrawIndexed(commandBuffer, subEntry.indexCount, 1, indexBase, 0, 0);
					indexBase += subEntry.indexCount;
				}
			}
		}
		if (appState.Scene.lastSurfaceRGBANoGlow >= 0)
		{
			vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.surfacesRGBANoGlow.pipeline);
			vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.surfacesRGBANoGlow.pipelineLayout, 0, 1, &sceneMatricesResources.descriptorSet, 0, nullptr);
			auto verticesBase = appState.Scene.verticesSize + appState.Scene.sortedSurfaceRGBANoGlowVerticesBase;
			auto attributesBase = appState.Scene.attributesSize + appState.Scene.sortedSurfaceRGBANoGlowAttributesBase;
			VkDeviceSize indexBase = 0;
			vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertices->buffer, &verticesBase);
			vkCmdBindVertexBuffers(commandBuffer, 1, 1, &attributes->buffer, &attributesBase);
			if (appState.Scene.sortedIndices16Size > 0)
			{
				vkCmdBindIndexBuffer(commandBuffer, indices16->buffer, appState.Scene.indices16Size + appState.Scene.sortedSurfaceRGBANoGlowIndicesBase, VK_INDEX_TYPE_UINT16);
			}
			else
			{
				vkCmdBindIndexBuffer(commandBuffer, indices32->buffer, appState.Scene.indices32Size + appState.Scene.sortedSurfaceRGBANoGlowIndicesBase, VK_INDEX_TYPE_UINT32);
			}
			pushConstants[0] = GammaCorrect(v_blend[0]);
			pushConstants[1] = GammaCorrect(v_blend[1]);
			pushConstants[2] = GammaCorrect(v_blend[2]);
			pushConstants[3] = GammaCorrect(v_blend[3]);
			vkCmdPushConstants(commandBuffer, appState.Scene.surfacesRGBANoGlow.pipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, 4 * sizeof(float), &pushConstants);
			for (auto& entry : appState.Scene.sorted.surfacesRGBANoGlow)
			{
				vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.surfacesRGBANoGlow.pipelineLayout, 1, 1, &entry.lightmap, 0, nullptr);
				for (auto& subEntry : entry.textures)
				{
					vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.surfacesRGBANoGlow.pipelineLayout, 2, 1, &subEntry.texture, 0, nullptr);
					vkCmdDrawIndexed(commandBuffer, subEntry.indexCount, 1, indexBase, 0, 0);
					indexBase += subEntry.indexCount;
				}
			}
		}
		if (appState.Scene.lastSurfaceRotated >= 0)
		{
			vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.surfacesRotated.pipeline);
			vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.surfacesRotated.pipelineLayout, 0, 1, &sceneMatricesAndColormapResources.descriptorSet, 0, nullptr);
			auto verticesBase = appState.Scene.verticesSize + appState.Scene.sortedSurfaceRotatedVerticesBase;
			auto attributesBase = appState.Scene.attributesSize + appState.Scene.sortedSurfaceRotatedAttributesBase;
			VkDeviceSize indexBase = 0;
			vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertices->buffer, &verticesBase);
			vkCmdBindVertexBuffers(commandBuffer, 1, 1, &attributes->buffer, &attributesBase);
			if (appState.Scene.sortedIndices16Size > 0)
			{
				vkCmdBindIndexBuffer(commandBuffer, indices16->buffer, appState.Scene.indices16Size + appState.Scene.sortedSurfaceRotatedIndicesBase, VK_INDEX_TYPE_UINT16);
			}
			else
			{
				vkCmdBindIndexBuffer(commandBuffer, indices32->buffer, appState.Scene.indices32Size + appState.Scene.sortedSurfaceRotatedIndicesBase, VK_INDEX_TYPE_UINT32);
			}
			for (auto& entry : appState.Scene.sorted.surfacesRotated)
			{
				vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.surfacesRotated.pipelineLayout, 1, 1, &entry.lightmap, 0, nullptr);
				for (auto& subEntry : entry.textures)
				{
					vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.surfacesRotated.pipelineLayout, 2, 1, &subEntry.texture, 0, nullptr);
					vkCmdDrawIndexed(commandBuffer, subEntry.indexCount, 1, indexBase, 0, 0);
					indexBase += subEntry.indexCount;
				}
			}
		}
		if (appState.Scene.lastSurfaceRotatedRGBA >= 0)
		{
			vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.surfacesRotatedRGBA.pipeline);
			vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.surfacesRotatedRGBA.pipelineLayout, 0, 1, &sceneMatricesResources.descriptorSet, 0, nullptr);
			auto verticesBase = appState.Scene.verticesSize + appState.Scene.sortedSurfaceRotatedRGBAVerticesBase;
			auto attributesBase = appState.Scene.attributesSize + appState.Scene.sortedSurfaceRotatedRGBAAttributesBase;
			VkDeviceSize indexBase = 0;
			vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertices->buffer, &verticesBase);
			vkCmdBindVertexBuffers(commandBuffer, 1, 1, &attributes->buffer, &attributesBase);
			if (appState.Scene.sortedIndices16Size > 0)
			{
				vkCmdBindIndexBuffer(commandBuffer, indices16->buffer, appState.Scene.indices16Size + appState.Scene.sortedSurfaceRotatedRGBAIndicesBase, VK_INDEX_TYPE_UINT16);
			}
			else
			{
				vkCmdBindIndexBuffer(commandBuffer, indices32->buffer, appState.Scene.indices32Size + appState.Scene.sortedSurfaceRotatedRGBAIndicesBase, VK_INDEX_TYPE_UINT32);
			}
			pushConstants[0] = GammaCorrect(v_blend[0]);
			pushConstants[1] = GammaCorrect(v_blend[1]);
			pushConstants[2] = GammaCorrect(v_blend[2]);
			pushConstants[3] = GammaCorrect(v_blend[3]);
			vkCmdPushConstants(commandBuffer, appState.Scene.surfacesRotatedRGBA.pipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, 4 * sizeof(float), &pushConstants);
			for (auto& entry : appState.Scene.sorted.surfacesRotatedRGBA)
			{
				vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.surfacesRotatedRGBA.pipelineLayout, 1, 1, &entry.lightmap, 0, nullptr);
				for (auto& subEntry : entry.textures)
				{
					vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.surfacesRotatedRGBA.pipelineLayout, 2, 1, &subEntry.texture, 0, nullptr);
					vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.surfacesRotatedRGBA.pipelineLayout, 3, 1, &subEntry.glowTexture, 0, nullptr);
					vkCmdDrawIndexed(commandBuffer, subEntry.indexCount, 1, indexBase, 0, 0);
					indexBase += subEntry.indexCount;
				}
			}
		}
		if (appState.Scene.lastSurfaceRotatedRGBANoGlow >= 0)
		{
			vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.surfacesRotatedRGBANoGlow.pipeline);
			vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.surfacesRotatedRGBANoGlow.pipelineLayout, 0, 1, &sceneMatricesResources.descriptorSet, 0, nullptr);
			auto verticesBase = appState.Scene.verticesSize + appState.Scene.sortedSurfaceRotatedRGBANoGlowVerticesBase;
			auto attributesBase = appState.Scene.attributesSize + appState.Scene.sortedSurfaceRotatedRGBANoGlowAttributesBase;
			VkDeviceSize indexBase = 0;
			vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertices->buffer, &verticesBase);
			vkCmdBindVertexBuffers(commandBuffer, 1, 1, &attributes->buffer, &attributesBase);
			if (appState.Scene.sortedIndices16Size > 0)
			{
				vkCmdBindIndexBuffer(commandBuffer, indices16->buffer, appState.Scene.indices16Size + appState.Scene.sortedSurfaceRotatedRGBANoGlowIndicesBase, VK_INDEX_TYPE_UINT16);
			}
			else
			{
				vkCmdBindIndexBuffer(commandBuffer, indices32->buffer, appState.Scene.indices32Size + appState.Scene.sortedSurfaceRotatedRGBANoGlowIndicesBase, VK_INDEX_TYPE_UINT32);
			}
			pushConstants[0] = GammaCorrect(v_blend[0]);
			pushConstants[1] = GammaCorrect(v_blend[1]);
			pushConstants[2] = GammaCorrect(v_blend[2]);
			pushConstants[3] = GammaCorrect(v_blend[3]);
			vkCmdPushConstants(commandBuffer, appState.Scene.surfacesRotatedRGBANoGlow.pipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, 4 * sizeof(float), &pushConstants);
			for (auto& entry : appState.Scene.sorted.surfacesRotatedRGBANoGlow)
			{
				vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.surfacesRotatedRGBANoGlow.pipelineLayout, 1, 1, &entry.lightmap, 0, nullptr);
				for (auto& subEntry : entry.textures)
				{
					vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.surfacesRotatedRGBANoGlow.pipelineLayout, 2, 1, &subEntry.texture, 0, nullptr);
					vkCmdDrawIndexed(commandBuffer, subEntry.indexCount, 1, indexBase, 0, 0);
					indexBase += subEntry.indexCount;
				}
			}
		}
		if (appState.Scene.lastFence >= 0)
		{
			vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.fences.pipeline);
			vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.fences.pipelineLayout, 0, 1, &sceneMatricesAndColormapResources.descriptorSet, 0, nullptr);
			auto verticesBase = appState.Scene.verticesSize + appState.Scene.sortedFenceVerticesBase;
			auto attributesBase = appState.Scene.attributesSize + appState.Scene.sortedFenceAttributesBase;
			VkDeviceSize indexBase = 0;
			vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertices->buffer, &verticesBase);
			vkCmdBindVertexBuffers(commandBuffer, 1, 1, &attributes->buffer, &attributesBase);
			if (appState.Scene.sortedIndices16Size > 0)
			{
				vkCmdBindIndexBuffer(commandBuffer, indices16->buffer, appState.Scene.indices16Size + appState.Scene.sortedFenceIndicesBase, VK_INDEX_TYPE_UINT16);
			}
			else
			{
				vkCmdBindIndexBuffer(commandBuffer, indices32->buffer, appState.Scene.indices32Size + appState.Scene.sortedFenceIndicesBase, VK_INDEX_TYPE_UINT32);
			}
			for (auto& entry : appState.Scene.sorted.fences)
			{
				vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.fences.pipelineLayout, 1, 1, &entry.lightmap, 0, nullptr);
				for (auto& subEntry : entry.textures)
				{
					vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.fences.pipelineLayout, 2, 1, &subEntry.texture, 0, nullptr);
					vkCmdDrawIndexed(commandBuffer, subEntry.indexCount, 1, indexBase, 0, 0);
					indexBase += subEntry.indexCount;
				}
			}
		}
		if (appState.Scene.lastFenceRGBA >= 0)
		{
			vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.fencesRGBA.pipeline);
			vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.fencesRGBA.pipelineLayout, 0, 1, &sceneMatricesResources.descriptorSet, 0, nullptr);
			auto verticesBase = appState.Scene.verticesSize + appState.Scene.sortedFenceRGBAVerticesBase;
			auto attributesBase = appState.Scene.attributesSize + appState.Scene.sortedFenceRGBAAttributesBase;
			VkDeviceSize indexBase = 0;
			vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertices->buffer, &verticesBase);
			vkCmdBindVertexBuffers(commandBuffer, 1, 1, &attributes->buffer, &attributesBase);
			vkCmdBindVertexBuffers(commandBuffer, 2, 1, &attributes->buffer, &attributesBase);
			if (appState.Scene.sortedIndices16Size > 0)
			{
				vkCmdBindIndexBuffer(commandBuffer, indices16->buffer, appState.Scene.indices16Size + appState.Scene.sortedFenceRGBAIndicesBase, VK_INDEX_TYPE_UINT16);
			}
			else
			{
				vkCmdBindIndexBuffer(commandBuffer, indices32->buffer, appState.Scene.indices32Size + appState.Scene.sortedFenceRGBAIndicesBase, VK_INDEX_TYPE_UINT32);
			}
			pushConstants[0] = GammaCorrect(v_blend[0]);
			pushConstants[1] = GammaCorrect(v_blend[1]);
			pushConstants[2] = GammaCorrect(v_blend[2]);
			pushConstants[3] = GammaCorrect(v_blend[3]);
			vkCmdPushConstants(commandBuffer, appState.Scene.fencesRGBA.pipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, 4 * sizeof(float), &pushConstants);
			for (auto& entry : appState.Scene.sorted.fencesRGBA)
			{
				vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.fencesRGBA.pipelineLayout, 1, 1, &entry.lightmap, 0, nullptr);
				for (auto& subEntry : entry.textures)
				{
					vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.fencesRGBA.pipelineLayout, 2, 1, &subEntry.texture, 0, nullptr);
					vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.fencesRGBA.pipelineLayout, 3, 1, &subEntry.glowTexture, 0, nullptr);
					vkCmdDrawIndexed(commandBuffer, subEntry.indexCount, 1, indexBase, 0, 0);
					indexBase += subEntry.indexCount;
				}
			}
		}
		if (appState.Scene.lastFenceRGBANoGlow >= 0)
		{
			vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.fencesRGBANoGlow.pipeline);
			vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.fencesRGBANoGlow.pipelineLayout, 0, 1, &sceneMatricesResources.descriptorSet, 0, nullptr);
			auto verticesBase = appState.Scene.verticesSize + appState.Scene.sortedFenceRGBANoGlowVerticesBase;
			auto attributesBase = appState.Scene.attributesSize + appState.Scene.sortedFenceRGBANoGlowAttributesBase;
			VkDeviceSize indexBase = 0;
			vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertices->buffer, &verticesBase);
			vkCmdBindVertexBuffers(commandBuffer, 1, 1, &attributes->buffer, &attributesBase);
			if (appState.Scene.sortedIndices16Size > 0)
			{
				vkCmdBindIndexBuffer(commandBuffer, indices16->buffer, appState.Scene.indices16Size + appState.Scene.sortedFenceRGBANoGlowIndicesBase, VK_INDEX_TYPE_UINT16);
			}
			else
			{
				vkCmdBindIndexBuffer(commandBuffer, indices32->buffer, appState.Scene.indices32Size + appState.Scene.sortedFenceRGBANoGlowIndicesBase, VK_INDEX_TYPE_UINT32);
			}
			pushConstants[0] = GammaCorrect(v_blend[0]);
			pushConstants[1] = GammaCorrect(v_blend[1]);
			pushConstants[2] = GammaCorrect(v_blend[2]);
			pushConstants[3] = GammaCorrect(v_blend[3]);
			vkCmdPushConstants(commandBuffer, appState.Scene.fencesRGBANoGlow.pipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, 4 * sizeof(float), &pushConstants);
			for (auto& entry : appState.Scene.sorted.fencesRGBANoGlow)
			{
				vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.fencesRGBANoGlow.pipelineLayout, 1, 1, &entry.lightmap, 0, nullptr);
				for (auto& subEntry : entry.textures)
				{
					vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.fencesRGBANoGlow.pipelineLayout, 2, 1, &subEntry.texture, 0, nullptr);
					vkCmdDrawIndexed(commandBuffer, subEntry.indexCount, 1, indexBase, 0, 0);
					indexBase += subEntry.indexCount;
				}
			}
		}
		if (appState.Scene.lastFenceRotated >= 0)
		{
			vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.fencesRotated.pipeline);
			vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.fencesRotated.pipelineLayout, 0, 1, &sceneMatricesAndColormapResources.descriptorSet, 0, nullptr);
			auto verticesBase = appState.Scene.verticesSize + appState.Scene.sortedFenceRotatedVerticesBase;
			auto attributesBase = appState.Scene.attributesSize + appState.Scene.sortedFenceRotatedAttributesBase;
			VkDeviceSize indexBase = 0;
			vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertices->buffer, &verticesBase);
			vkCmdBindVertexBuffers(commandBuffer, 1, 1, &attributes->buffer, &attributesBase);
			if (appState.Scene.sortedIndices16Size > 0)
			{
				vkCmdBindIndexBuffer(commandBuffer, indices16->buffer, appState.Scene.indices16Size + appState.Scene.sortedFenceRotatedIndicesBase, VK_INDEX_TYPE_UINT16);
			}
			else
			{
				vkCmdBindIndexBuffer(commandBuffer, indices32->buffer, appState.Scene.indices32Size + appState.Scene.sortedFenceRotatedIndicesBase, VK_INDEX_TYPE_UINT32);
			}
			for (auto& entry : appState.Scene.sorted.fencesRotated)
			{
				vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.fencesRotated.pipelineLayout, 1, 1, &entry.lightmap, 0, nullptr);
				for (auto& subEntry : entry.textures)
				{
					vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.fencesRotated.pipelineLayout, 2, 1, &subEntry.texture, 0, nullptr);
					vkCmdDrawIndexed(commandBuffer, subEntry.indexCount, 1, indexBase, 0, 0);
					indexBase += subEntry.indexCount;
				}
			}
		}
		if (appState.Scene.lastFenceRotatedRGBA >= 0)
		{
			vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.fencesRotatedRGBA.pipeline);
			vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.fencesRotatedRGBA.pipelineLayout, 0, 1, &sceneMatricesResources.descriptorSet, 0, nullptr);
			auto verticesBase = appState.Scene.verticesSize + appState.Scene.sortedFenceRotatedRGBAVerticesBase;
			auto attributesBase = appState.Scene.attributesSize + appState.Scene.sortedFenceRotatedRGBAAttributesBase;
			VkDeviceSize indexBase = 0;
			vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertices->buffer, &verticesBase);
			vkCmdBindVertexBuffers(commandBuffer, 1, 1, &attributes->buffer, &attributesBase);
			if (appState.Scene.sortedIndices16Size > 0)
			{
				vkCmdBindIndexBuffer(commandBuffer, indices16->buffer, appState.Scene.indices16Size + appState.Scene.sortedFenceRotatedRGBAIndicesBase, VK_INDEX_TYPE_UINT16);
			}
			else
			{
				vkCmdBindIndexBuffer(commandBuffer, indices32->buffer, appState.Scene.indices32Size + appState.Scene.sortedFenceRotatedRGBAIndicesBase, VK_INDEX_TYPE_UINT32);
			}
			pushConstants[0] = GammaCorrect(v_blend[0]);
			pushConstants[1] = GammaCorrect(v_blend[1]);
			pushConstants[2] = GammaCorrect(v_blend[2]);
			pushConstants[3] = GammaCorrect(v_blend[3]);
			vkCmdPushConstants(commandBuffer, appState.Scene.fencesRotatedRGBA.pipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, 4 * sizeof(float), &pushConstants);
			for (auto& entry : appState.Scene.sorted.fencesRotatedRGBA)
			{
				vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.fencesRotatedRGBA.pipelineLayout, 1, 1, &entry.lightmap, 0, nullptr);
				for (auto& subEntry : entry.textures)
				{
					vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.fencesRotatedRGBA.pipelineLayout, 2, 1, &subEntry.texture, 0, nullptr);
					vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.fencesRotatedRGBA.pipelineLayout, 3, 1, &subEntry.glowTexture, 0, nullptr);
					vkCmdDrawIndexed(commandBuffer, subEntry.indexCount, 1, indexBase, 0, 0);
					indexBase += subEntry.indexCount;
				}
			}
		}
		if (appState.Scene.lastFenceRotatedRGBANoGlow >= 0)
		{
			vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.fencesRotatedRGBANoGlow.pipeline);
			vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.fencesRotatedRGBANoGlow.pipelineLayout, 0, 1, &sceneMatricesResources.descriptorSet, 0, nullptr);
			auto verticesBase = appState.Scene.verticesSize + appState.Scene.sortedFenceRotatedRGBANoGlowVerticesBase;
			auto attributesBase = appState.Scene.attributesSize + appState.Scene.sortedFenceRotatedRGBANoGlowAttributesBase;
			VkDeviceSize indexBase = 0;
			vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertices->buffer, &verticesBase);
			vkCmdBindVertexBuffers(commandBuffer, 1, 1, &attributes->buffer, &attributesBase);
			if (appState.Scene.sortedIndices16Size > 0)
			{
				vkCmdBindIndexBuffer(commandBuffer, indices16->buffer, appState.Scene.indices16Size + appState.Scene.sortedFenceRotatedRGBANoGlowIndicesBase, VK_INDEX_TYPE_UINT16);
			}
			else
			{
				vkCmdBindIndexBuffer(commandBuffer, indices32->buffer, appState.Scene.indices32Size + appState.Scene.sortedFenceRotatedRGBANoGlowIndicesBase, VK_INDEX_TYPE_UINT32);
			}
			pushConstants[0] = GammaCorrect(v_blend[0]);
			pushConstants[1] = GammaCorrect(v_blend[1]);
			pushConstants[2] = GammaCorrect(v_blend[2]);
			pushConstants[3] = GammaCorrect(v_blend[3]);
			vkCmdPushConstants(commandBuffer, appState.Scene.fencesRotatedRGBANoGlow.pipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, 4 * sizeof(float), &pushConstants);
			for (auto& entry : appState.Scene.sorted.fencesRotatedRGBANoGlow)
			{
				vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.fencesRotatedRGBANoGlow.pipelineLayout, 1, 1, &entry.lightmap, 0, nullptr);
				for (auto& subEntry : entry.textures)
				{
					vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.fencesRotatedRGBANoGlow.pipelineLayout, 2, 1, &subEntry.texture, 0, nullptr);
					vkCmdDrawIndexed(commandBuffer, subEntry.indexCount, 1, indexBase, 0, 0);
					indexBase += subEntry.indexCount;
				}
			}
		}
		if (appState.Scene.lastTurbulent >= 0)
		{
			vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.turbulent.pipeline);
			vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.turbulent.pipelineLayout, 0, 1, &sceneMatricesAndPaletteResources.descriptorSet, 0, nullptr);
			auto verticesBase = appState.Scene.verticesSize + appState.Scene.sortedTurbulentVerticesBase;
			auto attributesBase = appState.Scene.attributesSize + appState.Scene.sortedTurbulentAttributesBase;
			VkDeviceSize indexBase = 0;
			vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertices->buffer, &verticesBase);
			vkCmdBindVertexBuffers(commandBuffer, 1, 1, &attributes->buffer, &attributesBase);
			if (appState.Scene.sortedIndices16Size > 0)
			{
				vkCmdBindIndexBuffer(commandBuffer, indices16->buffer, appState.Scene.indices16Size + appState.Scene.sortedTurbulentIndicesBase, VK_INDEX_TYPE_UINT16);
			}
			else
			{
				vkCmdBindIndexBuffer(commandBuffer, indices32->buffer, appState.Scene.indices32Size + appState.Scene.sortedTurbulentIndicesBase, VK_INDEX_TYPE_UINT32);
			}
			auto time = (float)cl.time;
			vkCmdPushConstants(commandBuffer, appState.Scene.turbulent.pipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(float), &time);
			for (auto& entry : appState.Scene.sorted.turbulent)
			{
				vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.turbulent.pipelineLayout, 1, 1, &entry.texture, 0, nullptr);
				vkCmdDrawIndexed(commandBuffer, entry.indexCount, 1, indexBase, 0, 0);
				indexBase += entry.indexCount;
			}
		}
		if (appState.Scene.lastTurbulentRGBA >= 0)
		{
			vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.turbulentRGBA.pipeline);
			vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.turbulentRGBA.pipelineLayout, 0, 1, &sceneMatricesResources.descriptorSet, 0, nullptr);
			auto verticesBase = appState.Scene.verticesSize + appState.Scene.sortedTurbulentRGBAVerticesBase;
			auto attributesBase = appState.Scene.attributesSize + appState.Scene.sortedTurbulentRGBAAttributesBase;
			VkDeviceSize indexBase = 0;
			vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertices->buffer, &verticesBase);
			vkCmdBindVertexBuffers(commandBuffer, 1, 1, &attributes->buffer, &attributesBase);
			if (appState.Scene.sortedIndices16Size > 0)
			{
				vkCmdBindIndexBuffer(commandBuffer, indices16->buffer, appState.Scene.indices16Size + appState.Scene.sortedTurbulentRGBAIndicesBase, VK_INDEX_TYPE_UINT16);
			}
			else
			{
				vkCmdBindIndexBuffer(commandBuffer, indices32->buffer, appState.Scene.indices32Size + appState.Scene.sortedTurbulentRGBAIndicesBase, VK_INDEX_TYPE_UINT32);
			}
			pushConstants[0] = GammaCorrect(v_blend[0]);
			pushConstants[1] = GammaCorrect(v_blend[1]);
			pushConstants[2] = GammaCorrect(v_blend[2]);
			pushConstants[3] = GammaCorrect(v_blend[3]);
			pushConstants[4] = (float)cl.time;
			vkCmdPushConstants(commandBuffer, appState.Scene.turbulentRGBA.pipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, 5 * sizeof(float), pushConstants);
			for (auto& entry : appState.Scene.sorted.turbulentRGBA)
			{
				vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.turbulentRGBA.pipelineLayout, 1, 1, &entry.texture, 0, nullptr);
				vkCmdDrawIndexed(commandBuffer, entry.indexCount, 1, indexBase, 0, 0);
				indexBase += entry.indexCount;
			}
		}
		if (appState.Scene.lastTurbulentLit >= 0)
		{
			vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.turbulentLit.pipeline);
			vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.turbulentLit.pipelineLayout, 0, 1, &sceneMatricesAndColormapResources.descriptorSet, 0, nullptr);
			auto verticesBase = appState.Scene.verticesSize + appState.Scene.sortedTurbulentLitVerticesBase;
			auto attributesBase = appState.Scene.attributesSize + appState.Scene.sortedTurbulentLitAttributesBase;
			VkDeviceSize indexBase = 0;
			vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertices->buffer, &verticesBase);
			vkCmdBindVertexBuffers(commandBuffer, 1, 1, &attributes->buffer, &attributesBase);
			if (appState.Scene.sortedIndices16Size > 0)
			{
				vkCmdBindIndexBuffer(commandBuffer, indices16->buffer, appState.Scene.indices16Size + appState.Scene.sortedTurbulentLitIndicesBase, VK_INDEX_TYPE_UINT16);
			}
			else
			{
				vkCmdBindIndexBuffer(commandBuffer, indices32->buffer, appState.Scene.indices32Size + appState.Scene.sortedTurbulentLitIndicesBase, VK_INDEX_TYPE_UINT32);
			}
			auto time = (float)cl.time;
			vkCmdPushConstants(commandBuffer, appState.Scene.turbulentLit.pipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(float), &time);
			for (auto& entry : appState.Scene.sorted.turbulentLit)
			{
				vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.turbulentLit.pipelineLayout, 1, 1, &entry.lightmap, 0, nullptr);
				for (auto& subEntry : entry.textures)
				{
					vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.turbulentLit.pipelineLayout, 2, 1, &subEntry.texture, 0, nullptr);
					vkCmdDrawIndexed(commandBuffer, subEntry.indexCount, 1, indexBase, 0, 0);
					indexBase += subEntry.indexCount;
				}
			}
		}
		if (appState.Scene.lastTurbulentLitRGBA >= 0)
		{
			vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.turbulentLitRGBA.pipeline);
			vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.turbulentLitRGBA.pipelineLayout, 0, 1, &sceneMatricesResources.descriptorSet, 0, nullptr);
			auto verticesBase = appState.Scene.verticesSize + appState.Scene.sortedTurbulentLitRGBAVerticesBase;
			auto attributesBase = appState.Scene.attributesSize + appState.Scene.sortedTurbulentLitRGBAAttributesBase;
			VkDeviceSize indexBase = 0;
			vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertices->buffer, &verticesBase);
			vkCmdBindVertexBuffers(commandBuffer, 1, 1, &attributes->buffer, &attributesBase);
			if (appState.Scene.sortedIndices16Size > 0)
			{
				vkCmdBindIndexBuffer(commandBuffer, indices16->buffer, appState.Scene.indices16Size + appState.Scene.sortedTurbulentLitRGBAIndicesBase, VK_INDEX_TYPE_UINT16);
			}
			else
			{
				vkCmdBindIndexBuffer(commandBuffer, indices32->buffer, appState.Scene.indices32Size + appState.Scene.sortedTurbulentLitRGBAIndicesBase, VK_INDEX_TYPE_UINT32);
			}
			pushConstants[0] = GammaCorrect(v_blend[0]);
			pushConstants[1] = GammaCorrect(v_blend[1]);
			pushConstants[2] = GammaCorrect(v_blend[2]);
			pushConstants[3] = GammaCorrect(v_blend[3]);
			pushConstants[4] = (float)cl.time;
			vkCmdPushConstants(commandBuffer, appState.Scene.turbulentLitRGBA.pipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, 5 * sizeof(float), pushConstants);
			for (auto& entry : appState.Scene.sorted.turbulentLitRGBA)
			{
				vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.turbulentLitRGBA.pipelineLayout, 1, 1, &entry.lightmap, 0, nullptr);
				for (auto& subEntry : entry.textures)
				{
					vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.turbulentLitRGBA.pipelineLayout, 2, 1, &subEntry.texture, 0, nullptr);
					vkCmdDrawIndexed(commandBuffer, subEntry.indexCount, 1, indexBase, 0, 0);
					indexBase += subEntry.indexCount;
				}
			}
		}
		if (appState.Scene.lastSprite >= 0)
		{
			vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.sprites.pipeline);
			vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertices->buffer, &texturedVertexBase);
			vkCmdBindVertexBuffers(commandBuffer, 1, 1, &attributes->buffer, &texturedAttributeBase);
			vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.sprites.pipelineLayout, 0, 1, &sceneMatricesAndPaletteResources.descriptorSet, 0, nullptr);
			SharedMemoryTexture* previousTexture = nullptr;
			for (auto i = 0; i <= appState.Scene.lastSprite; i++)
			{
				auto& loaded = appState.Scene.loadedSprites[i];
				auto texture = loaded.texture.texture;
				if (previousTexture != texture)
				{
					vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.sprites.pipelineLayout, 1, 1, &texture->descriptorSet, 0, nullptr);
					previousTexture = texture;
				}
				vkCmdDraw(commandBuffer, loaded.count, 1, loaded.firstVertex, 0);
			}
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
		if (appState.Scene.lastAlias >= 0)
		{
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
			for (auto i = 0; i <= appState.Scene.lastAlias; i++)
			{
				auto& loaded = appState.Scene.loadedAlias[i];
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
				VkDeviceSize attributeOffset = colormappedAttributeBase + loaded.firstAttribute * sizeof(float);
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
					auto colormap = loaded.colormapped.colormap.texture;
					if (colormapResources.bound[descriptorSetIndex] != colormap)
					{
						textureInfo.sampler = appState.Scene.samplers[colormap->mipCount];
						textureInfo.imageView = colormap->view;
						writes[0].dstSet = colormapResources.descriptorSets[descriptorSetIndex];
						vkUpdateDescriptorSets(appState.Device, 1, writes, 0, nullptr);
						colormapResources.bound[descriptorSetIndex] = colormap;
					}
					vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.alias.pipelineLayout, 1, 1, &colormapResources.descriptorSets[descriptorSetIndex], 0, nullptr);
					previousColormapDescriptorSet = colormapResources.descriptorSets[descriptorSetIndex];
					descriptorSetIndex++;
				}
				auto texture = loaded.colormapped.texture.texture;
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
		}
		if (appState.Scene.lastViewmodel >= 0)
		{
			vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.viewmodel.pipeline);
			vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.viewmodel.pipelineLayout, 0, 1, &sceneMatricesAndPaletteResources.descriptorSet, 0, nullptr);
			pushConstants[3] = 0;
			pushConstants[7] = 0;
			pushConstants[11] = 0;
			pushConstants[15] = 1;
			pushConstants[16] = appState.ViewmodelForwardX;
			pushConstants[17] = appState.ViewmodelForwardY;
			pushConstants[18] = appState.ViewmodelForwardZ;
			pushConstants[19] = 0;
			SharedMemoryBuffer* previousVertices = nullptr;
			SharedMemoryBuffer* previousTexCoords = nullptr;
			VkDescriptorSet previousColormapDescriptorSet = VK_NULL_HANDLE;
			SharedMemoryTexture* previousTexture = nullptr;
			SharedMemoryBuffer* previousIndices = nullptr;
			for (auto i = 0; i <= appState.Scene.lastViewmodel; i++)
			{
				auto& loaded = appState.Scene.loadedViewmodels[i];
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
				VkDeviceSize attributeOffset = colormappedAttributeBase + loaded.firstAttribute * sizeof(float);
				vkCmdBindVertexBuffers(commandBuffer, 2, 1, &attributes->buffer, &attributeOffset);
				SetPushConstants(loaded, pushConstants);
				vkCmdPushConstants(commandBuffer, appState.Scene.viewmodel.pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, 20 * sizeof(float), pushConstants);
				if (loaded.isHostColormap)
				{
					if (previousColormapDescriptorSet != host_colormapResources.descriptorSet)
					{
						vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.viewmodel.pipelineLayout, 1, 1, &host_colormapResources.descriptorSet, 0, nullptr);
						previousColormapDescriptorSet = host_colormapResources.descriptorSet;
					}
				}
				else
				{
					auto colormap = loaded.colormapped.colormap.texture;
					if (colormapResources.bound[descriptorSetIndex] != colormap)
					{
						textureInfo.sampler = appState.Scene.samplers[colormap->mipCount];
						textureInfo.imageView = colormap->view;
						writes[0].dstSet = colormapResources.descriptorSets[descriptorSetIndex];
						vkUpdateDescriptorSets(appState.Device, 1, writes, 0, nullptr);
						colormapResources.bound[descriptorSetIndex] = colormap;
					}
					vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.viewmodel.pipelineLayout, 1, 1, &colormapResources.descriptorSets[descriptorSetIndex], 0, nullptr);
					previousColormapDescriptorSet = colormapResources.descriptorSets[descriptorSetIndex];
					descriptorSetIndex++;
				}
				auto texture = loaded.colormapped.texture.texture;
				if (previousTexture != texture)
				{
					vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.viewmodel.pipelineLayout, 2, 1, &texture->descriptorSet, 0, nullptr);
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
		}
		if (appState.Scene.lastParticle >= 0)
		{
			vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.particle.pipeline);
			vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertices->buffer, &particlePositionBase);
			vkCmdBindVertexBuffers(commandBuffer, 1, 1, &colors->buffer, &appState.NoOffset);
			vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.particle.pipelineLayout, 0, 1, &sceneMatricesAndPaletteResources.descriptorSet, 0, nullptr);
			pushConstants[0] = appState.FromEngine.vright0;
			pushConstants[1] = appState.FromEngine.vright1;
			pushConstants[2] = appState.FromEngine.vright2;
			pushConstants[3] = 0;
			pushConstants[4] = appState.FromEngine.vup0;
			pushConstants[5] = appState.FromEngine.vup1;
			pushConstants[6] = appState.FromEngine.vup2;
			pushConstants[7] = 0;
			vkCmdPushConstants(commandBuffer, appState.Scene.particle.pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, 8 * sizeof(float), pushConstants);
			vkCmdDraw(commandBuffer, 6, appState.Scene.lastParticle + 1, 0, 0);
		}
		if (appState.Scene.lastColoredIndex8 >= 0 || appState.Scene.lastColoredIndex16 >= 0 || appState.Scene.lastColoredIndex32 >= 0)
		{
			vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.colored.pipeline);
			vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertices->buffer, &coloredVertexBase);
			vkCmdBindVertexBuffers(commandBuffer, 1, 1, &colors->buffer, &coloredColorBase);
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
		}
	}
	else
	{
		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.floorStrip.pipeline);
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
			textureInfo.sampler = appState.Scene.samplers[appState.Scene.floorTexture.mipCount];
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
		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.floorStrip.pipelineLayout, 0, 2, descriptorSets, 0, nullptr);
		if (appState.IndexTypeUInt8Enabled)
		{
			vkCmdBindIndexBuffer(commandBuffer, indices8->buffer, 0, VK_INDEX_TYPE_UINT8_EXT);
		}
		else
		{
			vkCmdBindIndexBuffer(commandBuffer, indices16->buffer, 0, VK_INDEX_TYPE_UINT16);
		}
		vkCmdDrawIndexed(commandBuffer, 4, 1, 0, 0, 0);
	}
	if (appState.Scene.controllerVerticesSize > 0)
	{
		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.floor.pipeline);
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
			textureInfo.sampler = appState.Scene.samplers[appState.Scene.controllerTexture.mipCount];
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
		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.floor.pipelineLayout, 0, 2, descriptorSets, 0, nullptr);
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
	}
}

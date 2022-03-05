#include "AppState.h"
#include "PerImage.h" // This header is specified second in the list to allow headers in AppState.h to include the core engine structs
#include "Constants.h"
#include "Utils.h"

float PerImage::GammaCorrect(float component)
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

void PerImage::LoadStagingBuffer(AppState& appState, PerFrame& perFrame, Buffer* stagingBuffer)
{
	VkDeviceSize offset = 0;
	auto loadedIndexBuffer = appState.Scene.indexBuffers.firstIndices8;
	while (loadedIndexBuffer != nullptr)
	{
		auto target = ((unsigned char*)stagingBuffer->mapped) + offset;
		auto face = (msurface_t*)loadedIndexBuffer->firstSource;
		auto model = (model_t*)loadedIndexBuffer->secondSource;
		auto edge = model->surfedges[face->firstedge];
		unsigned char index;
		if (edge >= 0)
		{
			index = model->edges[edge].v[0];
		}
		else
		{
			index = model->edges[-edge].v[1];
		}
		*target++ = index;
		auto next_front = 0;
		auto next_back = face->numedges;
		auto use_back = false;
		for (auto i = 1; i < face->numedges; i++)
		{
			if (use_back)
			{
				next_back--;
				edge = model->surfedges[face->firstedge + next_back];
			}
			else
			{
				next_front++;
				edge = model->surfedges[face->firstedge + next_front];
			}
			use_back = !use_back;
			if (edge >= 0)
			{
				index = model->edges[edge].v[0];
			}
			else
			{
				index = model->edges[-edge].v[1];
			}
			*target++ = index;
		}
		offset += loadedIndexBuffer->size;
		loadedIndexBuffer = loadedIndexBuffer->next;
	}
	auto loadedAliasIndexBuffer = appState.Scene.indexBuffers.firstAliasIndices8;
	while (loadedAliasIndexBuffer != nullptr)
	{
		auto target = ((unsigned char*)stagingBuffer->mapped) + offset;
		auto aliashdr = (aliashdr_t *)loadedAliasIndexBuffer->firstSource;
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
	loadedIndexBuffer = appState.Scene.indexBuffers.firstIndices16;
	while (loadedIndexBuffer != nullptr)
	{
		auto target = (uint16_t*)(((unsigned char*)stagingBuffer->mapped) + offset);
		auto face = (msurface_t*)loadedIndexBuffer->firstSource;
		auto model = (model_t*)loadedIndexBuffer->secondSource;
		auto edge = model->surfedges[face->firstedge];
		uint16_t index;
		if (edge >= 0)
		{
			index = model->edges[edge].v[0];
		}
		else
		{
			index = model->edges[-edge].v[1];
		}
		*target++ = index;
		auto next_front = 0;
		auto next_back = face->numedges;
		auto use_back = false;
		for (auto i = 1; i < face->numedges; i++)
		{
			if (use_back)
			{
				next_back--;
				edge = model->surfedges[face->firstedge + next_back];
			}
			else
			{
				next_front++;
				edge = model->surfedges[face->firstedge + next_front];
			}
			use_back = !use_back;
			if (edge >= 0)
			{
				index = model->edges[edge].v[0];
			}
			else
			{
				index = model->edges[-edge].v[1];
			}
			*target++ = index;
		}
		offset += loadedIndexBuffer->size;
		loadedIndexBuffer = loadedIndexBuffer->next;
	}
	loadedAliasIndexBuffer = appState.Scene.indexBuffers.firstAliasIndices16;
	while (loadedAliasIndexBuffer != nullptr)
	{
		auto target = (uint16_t*)(((unsigned char*)stagingBuffer->mapped) + offset);
		auto aliashdr = (aliashdr_t *)loadedAliasIndexBuffer->firstSource;
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
	loadedIndexBuffer = appState.Scene.indexBuffers.firstIndices32;
	while (loadedIndexBuffer != nullptr)
	{
		auto target = (uint32_t*)(((unsigned char*)stagingBuffer->mapped) + offset);
		auto face = (msurface_t*)loadedIndexBuffer->firstSource;
		auto model = (model_t*)loadedIndexBuffer->secondSource;
		auto edge = model->surfedges[face->firstedge];
		uint32_t index;
		if (edge >= 0)
		{
			index = model->edges[edge].v[0];
		}
		else
		{
			index = model->edges[-edge].v[1];
		}
		*target++ = index;
		auto next_front = 0;
		auto next_back = face->numedges;
		auto use_back = false;
		for (auto i = 1; i < face->numedges; i++)
		{
			if (use_back)
			{
				next_back--;
				edge = model->surfedges[face->firstedge + next_back];
			}
			else
			{
				next_front++;
				edge = model->surfedges[face->firstedge + next_front];
			}
			use_back = !use_back;
			if (edge >= 0)
			{
				index = model->edges[edge].v[0];
			}
			else
			{
				index = model->edges[-edge].v[1];
			}
			*target++ = index;
		}
		offset += loadedIndexBuffer->size;
		loadedIndexBuffer = loadedIndexBuffer->next;
	}
	loadedAliasIndexBuffer = appState.Scene.indexBuffers.firstAliasIndices32;
	while (loadedAliasIndexBuffer != nullptr)
	{
		auto target = (uint32_t*)(((unsigned char*)stagingBuffer->mapped) + offset);
		auto aliashdr = (aliashdr_t *)loadedAliasIndexBuffer->firstSource;
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
		auto target = (float*)(((unsigned char*)stagingBuffer->mapped) + offset);
		auto source = (trivertx_t*)loadedBuffer->source;
		for (auto i = 0; i < loadedBuffer->size; i += 2 * 4 * sizeof(float))
		{
			auto x = (float)(source->v[0]);
			auto y = (float)(source->v[1]);
			auto z = (float)(source->v[2]);
			*target++ = x;
			*target++ = z;
			*target++ = -y;
			*target++ = 1;
			*target++ = x;
			*target++ = z;
			*target++ = -y;
			*target++ = 1;
			source++;
		}
		offset += loadedBuffer->size;
		loadedBuffer = loadedBuffer->next;
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
	while (offset % 4 != 0)
	{
		offset++;
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
		perFrame.controllerVertexBase = appState.Scene.floorVerticesSize;
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
		perFrame.texturedVertexBase = perFrame.controllerVertexBase + appState.Scene.controllerVerticesSize;
		memcpy(((unsigned char*)stagingBuffer->mapped) + offset, d_lists.textured_vertices.data(), appState.Scene.texturedVerticesSize);
		offset += appState.Scene.texturedVerticesSize;
		perFrame.particlePositionBase = perFrame.texturedVertexBase + appState.Scene.texturedVerticesSize;
		memcpy(((unsigned char*)stagingBuffer->mapped) + offset, d_lists.particle_positions.data(), appState.Scene.particlePositionsSize);
		offset += appState.Scene.particlePositionsSize;
		perFrame.coloredVertexBase = perFrame.particlePositionBase + appState.Scene.particlePositionsSize;
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
	perFrame.controllerAttributeBase = appState.Scene.floorAttributesSize;
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
	perFrame.texturedAttributeBase = perFrame.controllerAttributeBase + appState.Scene.controllerAttributesSize;
	memcpy(((unsigned char*)stagingBuffer->mapped) + offset, d_lists.textured_attributes.data(), appState.Scene.texturedAttributesSize);
	offset += appState.Scene.texturedAttributesSize;
	perFrame.colormappedAttributeBase = perFrame.texturedAttributeBase + appState.Scene.texturedAttributesSize;
	memcpy(((unsigned char*)stagingBuffer->mapped) + offset, d_lists.colormapped_attributes.data(), appState.Scene.colormappedLightsSize);
	offset += appState.Scene.colormappedLightsSize;
	memcpy(((unsigned char*)stagingBuffer->mapped) + offset, d_lists.particle_colors.data(), appState.Scene.particleColorsSize);
	offset += appState.Scene.particleColorsSize;
	perFrame.coloredColorBase = appState.Scene.particleColorsSize;
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
				*target++ = 2;
				*target++ = 2;
				*target++ = 3;
				*target++ = 0;
				offset += appState.Scene.floorIndicesSize;
			}
			perFrame.controllerIndexBase = appState.Scene.floorIndicesSize;
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
			perFrame.coloredIndex8Base = perFrame.controllerIndexBase + appState.Scene.controllerIndicesSize;
			memcpy(((unsigned char*)stagingBuffer->mapped) + offset, d_lists.colored_indices8.data(), appState.Scene.coloredIndices8Size);
			offset += appState.Scene.coloredIndices8Size;
			while (offset % 4 != 0)
			{
				offset++;
			}
		}
		if (appState.Scene.indices16Size > 0)
		{
			perFrame.coloredIndex16Base = 0;
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
			*target++ = 2;
			*target++ = 2;
			*target++ = 3;
			*target++ = 0;
			offset += appState.Scene.floorIndicesSize;
		}
		perFrame.controllerIndexBase = appState.Scene.floorIndicesSize;
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
		perFrame.coloredIndex16Base = perFrame.controllerIndexBase + appState.Scene.controllerIndicesSize;
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
		SortedSurfaces::LoadVertices(appState.Scene.sorted.surfacesRotated, appState.Scene.loadedSurfacesRotated, stagingBuffer, offset);
		SortedSurfaces::LoadVertices(appState.Scene.sorted.fences, appState.Scene.loadedFences, stagingBuffer, offset);
		SortedSurfaces::LoadVertices(appState.Scene.sorted.fencesRotated, appState.Scene.loadedFencesRotated, stagingBuffer, offset);
		SortedSurfaces::LoadVertices(appState.Scene.sorted.turbulent, appState.Scene.loadedTurbulent, stagingBuffer, offset);
		SortedSurfaces::LoadVertices(appState.Scene.sorted.turbulentLit, appState.Scene.loadedTurbulentLit, stagingBuffer, offset);
	}
	if (appState.Scene.sortedAttributesSize > 0)
	{
		SortedSurfaces::LoadAttributes(appState.Scene.sorted.surfaces, appState.Scene.loadedSurfaces, stagingBuffer, offset);
		SortedSurfaces::LoadAttributes(appState.Scene.sorted.surfacesRotated, appState.Scene.loadedSurfacesRotated, stagingBuffer, offset);
		SortedSurfaces::LoadAttributes(appState.Scene.sorted.fences, appState.Scene.loadedFences, stagingBuffer, offset);
		SortedSurfaces::LoadAttributes(appState.Scene.sorted.fencesRotated, appState.Scene.loadedFencesRotated, stagingBuffer, offset);
		SortedSurfaces::LoadAttributes(appState.Scene.sorted.turbulent, appState.Scene.loadedTurbulent, stagingBuffer, offset);
		SortedSurfaces::LoadAttributes(appState.Scene.sorted.turbulentLit, appState.Scene.loadedTurbulentLit, stagingBuffer, offset);
	}
	if (appState.Scene.sortedIndicesCount > 0)
	{
		if (appState.Scene.sortedIndices16Size > 0)
		{
			SortedSurfaces::LoadIndices16(appState.Scene.sorted.surfaces, appState.Scene.loadedSurfaces, stagingBuffer, offset);
			SortedSurfaces::LoadIndices16(appState.Scene.sorted.surfacesRotated, appState.Scene.loadedSurfacesRotated, stagingBuffer, offset);
			SortedSurfaces::LoadIndices16(appState.Scene.sorted.fences, appState.Scene.loadedFences, stagingBuffer, offset);
			SortedSurfaces::LoadIndices16(appState.Scene.sorted.fencesRotated, appState.Scene.loadedFencesRotated, stagingBuffer, offset);
			SortedSurfaces::LoadIndices16(appState.Scene.sorted.turbulent, appState.Scene.loadedTurbulent, stagingBuffer, offset);
			SortedSurfaces::LoadIndices16(appState.Scene.sorted.turbulentLit, appState.Scene.loadedTurbulentLit, stagingBuffer, offset);
			while (offset % 4 != 0)
			{
				offset++;
			}
		}
		else
		{
			SortedSurfaces::LoadIndices32(appState.Scene.sorted.surfaces, appState.Scene.loadedSurfaces, stagingBuffer, offset);
			SortedSurfaces::LoadIndices32(appState.Scene.sorted.surfacesRotated, appState.Scene.loadedSurfacesRotated, stagingBuffer, offset);
			SortedSurfaces::LoadIndices32(appState.Scene.sorted.fences, appState.Scene.loadedFences, stagingBuffer, offset);
			SortedSurfaces::LoadIndices32(appState.Scene.sorted.fencesRotated, appState.Scene.loadedFencesRotated, stagingBuffer, offset);
			SortedSurfaces::LoadIndices32(appState.Scene.sorted.turbulent, appState.Scene.loadedTurbulent, stagingBuffer, offset);
			SortedSurfaces::LoadIndices32(appState.Scene.sorted.turbulentLit, appState.Scene.loadedTurbulentLit, stagingBuffer, offset);
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
		auto source = r_skysource;
		auto target = ((unsigned char*)stagingBuffer->mapped) + offset;
		for (auto i = 0; i < 128; i++)
		{
			memcpy(target, source, 128);
			source += 256;
			target += 128;
		}
	}
}

void PerImage::FillColormapTextures(AppState& appState, PerFrame& perFrame, LoadedAlias& loaded)
{
	if (!loaded.isHostColormap)
	{
		Texture* texture;
		if (perFrame.colormaps.oldTextures != nullptr)
		{
			texture = perFrame.colormaps.oldTextures;
			perFrame.colormaps.oldTextures = texture->next;
		}
		else
		{
			texture = new Texture();
			texture->Create(appState, 256, 64, VK_FORMAT_R8_UINT, 1, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
		}
		texture->Fill(appState, appState.Scene.stagingBuffer);
		appState.Scene.stagingBuffer.offset += 16384;
		perFrame.colormaps.MoveToFront(texture);
		loaded.colormapped.colormap.texture = texture;
		perFrame.colormapCount++;
	}
}

void PerImage::FillFromStagingBuffer(AppState& appState, Buffer* stagingBuffer, LoadedIndexBuffer* first, VkBufferCopy& bufferCopy, SharedMemoryBuffer*& previousBuffer) const
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

void PerImage::FillAliasFromStagingBuffer(AppState& appState, Buffer* stagingBuffer, LoadedIndexBuffer* first, VkBufferCopy& bufferCopy, SharedMemoryBuffer*& previousBuffer) const
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

void PerImage::FillFromStagingBuffer(AppState& appState, Buffer* stagingBuffer, LoadedSharedMemoryBuffer* first, VkBufferCopy& bufferCopy) const
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

void PerImage::FillFromStagingBuffer(AppState& appState, PerFrame& perFrame, Buffer* stagingBuffer) const
{
	appState.Scene.stagingBuffer.lastBarrier = -1;
	appState.Scene.stagingBuffer.descriptorSetsInUse.clear();

	VkBufferCopy bufferCopy { };

	SharedMemoryBuffer* previousBuffer = nullptr;
	FillFromStagingBuffer(appState, stagingBuffer, appState.Scene.indexBuffers.firstIndices8, bufferCopy, previousBuffer);
	FillAliasFromStagingBuffer(appState, stagingBuffer, appState.Scene.indexBuffers.firstAliasIndices8, bufferCopy, previousBuffer);

	while (bufferCopy.srcOffset % 4 != 0)
	{
		bufferCopy.srcOffset++;
	}

	FillFromStagingBuffer(appState, stagingBuffer, appState.Scene.indexBuffers.firstIndices16, bufferCopy, previousBuffer);
	FillAliasFromStagingBuffer(appState, stagingBuffer, appState.Scene.indexBuffers.firstAliasIndices16, bufferCopy, previousBuffer);

	while (bufferCopy.srcOffset % 4 != 0)
	{
		bufferCopy.srcOffset++;
	}

	FillFromStagingBuffer(appState, stagingBuffer, appState.Scene.indexBuffers.firstIndices32, bufferCopy, previousBuffer);
	FillAliasFromStagingBuffer(appState, stagingBuffer, appState.Scene.indexBuffers.firstAliasIndices32, bufferCopy, previousBuffer);

	bufferCopy.dstOffset = 0;

	FillFromStagingBuffer(appState, stagingBuffer, appState.Scene.buffers.firstAliasVertices, bufferCopy);

	auto loadedTexCoordsBuffer = appState.Scene.buffers.firstAliasTexCoords;
	while (loadedTexCoordsBuffer != nullptr)
	{
		bufferCopy.size = loadedTexCoordsBuffer->size;
		vkCmdCopyBuffer(commandBuffer, stagingBuffer->buffer, loadedTexCoordsBuffer->buffer->buffer, 1, &bufferCopy);
		bufferCopy.srcOffset += bufferCopy.size;
		appState.Scene.AddToBufferBarrier(loadedTexCoordsBuffer->buffer->buffer);
		loadedTexCoordsBuffer = loadedTexCoordsBuffer->next;
	}

	while (bufferCopy.srcOffset % 4 != 0)
	{
		bufferCopy.srcOffset++;
	}

	if (appState.Scene.verticesSize > 0)
	{
		bufferCopy.size = appState.Scene.verticesSize;
		vkCmdCopyBuffer(commandBuffer, stagingBuffer->buffer, perFrame.vertices->buffer, 1, &bufferCopy);
		bufferCopy.srcOffset += bufferCopy.size;
		appState.Scene.AddToBufferBarrier(perFrame.vertices->buffer);
	}
	if (appState.Scene.attributesSize > 0)
	{
		bufferCopy.size = appState.Scene.attributesSize;
		vkCmdCopyBuffer(commandBuffer, stagingBuffer->buffer, perFrame.attributes->buffer, 1, &bufferCopy);
		bufferCopy.srcOffset += bufferCopy.size;
		appState.Scene.AddToBufferBarrier(perFrame.attributes->buffer);
	}
	if (appState.Scene.colorsSize > 0)
	{
		bufferCopy.size = appState.Scene.colorsSize;
		vkCmdCopyBuffer(commandBuffer, stagingBuffer->buffer, perFrame.colors->buffer, 1, &bufferCopy);
		bufferCopy.srcOffset += bufferCopy.size;
		appState.Scene.AddToBufferBarrier(perFrame.colors->buffer);
	}

	if (appState.Scene.indices8Size > 0)
	{
		bufferCopy.size = appState.Scene.indices8Size;
		vkCmdCopyBuffer(commandBuffer, stagingBuffer->buffer, perFrame.indices8->buffer, 1, &bufferCopy);
		bufferCopy.srcOffset += bufferCopy.size;
		appState.Scene.AddToBufferBarrier(perFrame.indices8->buffer);

		while (bufferCopy.srcOffset % 4 != 0)
		{
			bufferCopy.srcOffset++;
		}
	}
	if (appState.Scene.indices16Size > 0)
	{
		bufferCopy.size = appState.Scene.indices16Size;
		vkCmdCopyBuffer(commandBuffer, stagingBuffer->buffer, perFrame.indices16->buffer, 1, &bufferCopy);
		bufferCopy.srcOffset += bufferCopy.size;
		appState.Scene.AddToBufferBarrier(perFrame.indices16->buffer);

		while (bufferCopy.srcOffset % 4 != 0)
		{
			bufferCopy.srcOffset++;
		}
	}
	if (appState.Scene.indices32Size > 0)
	{
		bufferCopy.size = appState.Scene.indices32Size;
		vkCmdCopyBuffer(commandBuffer, stagingBuffer->buffer, perFrame.indices32->buffer, 1, &bufferCopy);
		bufferCopy.srcOffset += bufferCopy.size;
		appState.Scene.AddToBufferBarrier(perFrame.indices32->buffer);
	}

	if (appState.Scene.sortedVerticesSize > 0)
	{
		bufferCopy.size = appState.Scene.sortedVerticesSize;
		bufferCopy.dstOffset = appState.Scene.verticesSize;
		vkCmdCopyBuffer(commandBuffer, stagingBuffer->buffer, perFrame.vertices->buffer, 1, &bufferCopy);
		bufferCopy.dstOffset = 0;
		bufferCopy.srcOffset += bufferCopy.size;
		if (appState.Scene.verticesSize == 0)
		{
			appState.Scene.AddToBufferBarrier(perFrame.vertices->buffer);
		}
	}
	if (appState.Scene.sortedAttributesSize > 0)
	{
		bufferCopy.size = appState.Scene.sortedAttributesSize;
		bufferCopy.dstOffset = appState.Scene.attributesSize;
		vkCmdCopyBuffer(commandBuffer, stagingBuffer->buffer, perFrame.attributes->buffer, 1, &bufferCopy);
		bufferCopy.dstOffset = 0;
		bufferCopy.srcOffset += bufferCopy.size;
		if (appState.Scene.attributesSize == 0)
		{
			appState.Scene.AddToBufferBarrier(perFrame.attributes->buffer);
		}
	}
	if (appState.Scene.sortedIndicesCount > 0)
	{
		if (appState.Scene.sortedIndices16Size > 0)
		{
			bufferCopy.size = appState.Scene.sortedIndices16Size;
			bufferCopy.dstOffset = appState.Scene.indices16Size;
			vkCmdCopyBuffer(commandBuffer, stagingBuffer->buffer, perFrame.indices16->buffer, 1, &bufferCopy);
			bufferCopy.dstOffset = 0;
			bufferCopy.srcOffset += bufferCopy.size;
			if (appState.Scene.indices16Size == 0)
			{
				appState.Scene.AddToBufferBarrier(perFrame.indices16->buffer);
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
			vkCmdCopyBuffer(commandBuffer, stagingBuffer->buffer, perFrame.indices32->buffer, 1, &bufferCopy);
			bufferCopy.dstOffset = 0;
			bufferCopy.srcOffset += bufferCopy.size;
			if (appState.Scene.indices32Size == 0)
			{
				appState.Scene.AddToBufferBarrier(perFrame.indices32->buffer);
			}
		}
	}

	if (appState.Scene.paletteSize > 0)
	{
		bufferCopy.size = appState.Scene.paletteSize;
		vkCmdCopyBuffer(commandBuffer, stagingBuffer->buffer, perFrame.palette->buffer, 1, &bufferCopy);
		bufferCopy.srcOffset += bufferCopy.size;
		appState.Scene.AddToBufferBarrier(perFrame.palette->buffer);
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
		perFrame.host_colormap->Fill(appState, appState.Scene.stagingBuffer);
		appState.Scene.stagingBuffer.offset += appState.Scene.host_colormapSize;
	}
	auto loadedLightmap = appState.Scene.lightmaps.first;
	while (loadedLightmap != nullptr)
	{
		loadedLightmap->lightmap->Fill(appState, appState.Scene.stagingBuffer);
		appState.Scene.stagingBuffer.offset += loadedLightmap->size;
		loadedLightmap = loadedLightmap->next;
	}
	auto loadedTexture = appState.Scene.textures.first;
	while (loadedTexture != nullptr)
	{
		loadedTexture->texture->FillMipmapped(appState, appState.Scene.stagingBuffer);
		appState.Scene.stagingBuffer.offset += loadedTexture->size;
		loadedTexture = loadedTexture->next;
	}

	for (auto i = 0; i <= appState.Scene.lastAlias; i++)
	{
		FillColormapTextures(appState, perFrame, appState.Scene.loadedAlias[i]);
	}
	for (auto i = 0; i <= appState.Scene.lastViewmodel; i++)
	{
		FillColormapTextures(appState, perFrame, appState.Scene.loadedViewmodels[i]);
	}

	if (appState.Scene.lastSky >= 0)
	{
		perFrame.sky->FillMipmapped(appState, appState.Scene.stagingBuffer);
		appState.Scene.stagingBuffer.offset += appState.Scene.skySize;
	}

	if (appState.Scene.stagingBuffer.lastBarrier >= 0)
	{
		vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, appState.Scene.stagingBuffer.lastBarrier + 1, appState.Scene.stagingBuffer.imageBarriers.data());
	}
}

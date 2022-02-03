#include "AppState.h"
#include "PerImage.h" // This header is specified second in the list to allow headers in AppState.h to include the core engine structs
#include "Constants.h"
#include "Utils.h"

void PerImage::Reset(AppState& appState)
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

VkDeviceSize PerImage::GetStagingBufferSize(AppState& appState)
{
	appState.Scene.lastSurface = d_lists.last_surface;
	appState.Scene.lastSurfaceRotated = d_lists.last_surface_rotated;
	appState.Scene.lastFence = d_lists.last_fence;
	appState.Scene.lastFenceRotated = d_lists.last_fence_rotated;
	appState.Scene.lastSprite = d_lists.last_sprite;
	appState.Scene.lastTurbulent = d_lists.last_turbulent;
	appState.Scene.lastTurbulentLit = d_lists.last_turbulent_lit;
	appState.Scene.lastTurbulentRotated = d_lists.last_turbulent_rotated;
	appState.Scene.lastTurbulentRotatedLit = d_lists.last_turbulent_rotated_lit;
	appState.Scene.lastAlias = d_lists.last_alias;
	appState.Scene.lastViewmodel = d_lists.last_viewmodel;
	appState.Scene.lastParticle = d_lists.last_particle_color;
	appState.Scene.lastColoredIndex8 = d_lists.last_colored_index8;
	appState.Scene.lastColoredIndex16 = d_lists.last_colored_index16;
	appState.Scene.lastColoredIndex32 = d_lists.last_colored_index32;
	appState.Scene.lastSky = d_lists.last_sky;
	appState.Scene.vright0 = d_lists.vright0;
	appState.Scene.vright1 = d_lists.vright1;
	appState.Scene.vright2 = d_lists.vright2;
	appState.Scene.vup0 = d_lists.vup0;
	appState.Scene.vup1 = d_lists.vup1;
	appState.Scene.vup2 = d_lists.vup2;
	if (appState.Scene.lastSurface >= appState.Scene.loadedSurfaces.size())
	{
		appState.Scene.loadedSurfaces.resize(appState.Scene.lastSurface + 1);
	}
	if (appState.Scene.lastSurfaceRotated >= appState.Scene.loadedSurfacesRotated.size())
	{
		appState.Scene.loadedSurfacesRotated.resize(appState.Scene.lastSurfaceRotated + 1);
	}
	if (appState.Scene.lastFence >= appState.Scene.loadedFences.size())
	{
		appState.Scene.loadedFences.resize(appState.Scene.lastFence + 1);
	}
	if (appState.Scene.lastFenceRotated >= appState.Scene.loadedFencesRotated.size())
	{
		appState.Scene.loadedFencesRotated.resize(appState.Scene.lastFenceRotated + 1);
	}
	if (appState.Scene.lastSprite >= appState.Scene.loadedSprites.size())
	{
		appState.Scene.loadedSprites.resize(appState.Scene.lastSprite + 1);
	}
	if (appState.Scene.lastTurbulent >= appState.Scene.loadedTurbulent.size())
	{
		appState.Scene.loadedTurbulent.resize(appState.Scene.lastTurbulent + 1);
	}
	if (appState.Scene.lastTurbulentLit >= appState.Scene.loadedTurbulentLit.size())
	{
		appState.Scene.loadedTurbulentLit.resize(appState.Scene.lastTurbulentLit + 1);
	}
	if (appState.Scene.lastTurbulentRotated >= appState.Scene.loadedTurbulentRotated.size())
	{
		appState.Scene.loadedTurbulentRotated.resize(appState.Scene.lastTurbulentRotated + 1);
	}
	if (appState.Scene.lastTurbulentRotatedLit >= appState.Scene.loadedTurbulentRotatedLit.size())
	{
		appState.Scene.loadedTurbulentRotatedLit.resize(appState.Scene.lastTurbulentRotatedLit + 1);
	}
	if (appState.Scene.lastAlias >= appState.Scene.loadedAlias.size())
	{
		appState.Scene.loadedAlias.resize(appState.Scene.lastAlias + 1);
	}
	if (appState.Scene.lastViewmodel >= appState.Scene.loadedViewmodels.size())
	{
		appState.Scene.loadedViewmodels.resize(appState.Scene.lastViewmodel + 1);
	}
	auto size = matrices.size;
	appState.Scene.paletteSize = 0;
	if (palette == nullptr || paletteChanged != pal_changed)
	{
		if (palette == nullptr)
		{
			palette = new Buffer();
			palette->CreateUniformBuffer(appState, 256 * 4 * sizeof(float));
		}
		appState.Scene.paletteSize = palette->size;
		size += appState.Scene.paletteSize;
		paletteChanged = pal_changed;
	}
	appState.Scene.host_colormapSize = 0;
	if (!::host_colormap.empty() && host_colormap == nullptr)
	{
		host_colormap = new Texture();
		host_colormap->Create(appState, 256, 64, VK_FORMAT_R8_UINT, 1, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
		appState.Scene.host_colormapSize = 16384;
		size += appState.Scene.host_colormapSize;
	}
	appState.Scene.previousVertexes = nullptr;
	appState.Scene.previousTexture = nullptr;
	for (auto i = 0; i <= d_lists.last_surface; i++)
	{
		appState.Scene.GetStagingBufferSize(appState, d_lists.surfaces[i], appState.Scene.loadedSurfaces[i], size);
	}
	appState.Scene.previousVertexes = nullptr;
	appState.Scene.previousTexture = nullptr;
	for (auto i = 0; i <= appState.Scene.lastSurfaceRotated; i++)
	{
		appState.Scene.GetStagingBufferSize(appState, d_lists.surfaces_rotated[i], appState.Scene.loadedSurfacesRotated[i], size);
	}
	appState.Scene.previousVertexes = nullptr;
	appState.Scene.previousTexture = nullptr;
	for (auto i = 0; i <= appState.Scene.lastFence; i++)
	{
		appState.Scene.GetStagingBufferSize(appState, d_lists.fences[i], appState.Scene.loadedFences[i], size);
	}
	appState.Scene.previousVertexes = nullptr;
	appState.Scene.previousTexture = nullptr;
	for (auto i = 0; i <= appState.Scene.lastFenceRotated; i++)
	{
		appState.Scene.GetStagingBufferSize(appState, d_lists.fences_rotated[i], appState.Scene.loadedFencesRotated[i], size);
	}
	appState.Scene.previousTexture = nullptr;
	for (auto i = 0; i <= appState.Scene.lastSprite; i++)
	{
		appState.Scene.GetStagingBufferSize(appState, d_lists.sprites[i], appState.Scene.loadedSprites[i], size);
	}
	appState.Scene.previousVertexes = nullptr;
	appState.Scene.previousTexture = nullptr;
	for (auto i = 0; i <= appState.Scene.lastTurbulent; i++)
	{
		appState.Scene.GetStagingBufferSize(appState, d_lists.turbulent[i], appState.Scene.loadedTurbulent[i], size);
	}
	appState.Scene.previousVertexes = nullptr;
	appState.Scene.previousTexture = nullptr;
	for (auto i = 0; i <= appState.Scene.lastTurbulentLit; i++)
	{
		appState.Scene.GetStagingBufferSize(appState, d_lists.turbulent_lit[i], appState.Scene.loadedTurbulentLit[i], size);
	}
	appState.Scene.previousVertexes = nullptr;
	appState.Scene.previousTexture = nullptr;
	for (auto i = 0; i <= appState.Scene.lastTurbulentRotated; i++)
	{
		appState.Scene.GetStagingBufferSize(appState, d_lists.turbulent_rotated[i], appState.Scene.loadedTurbulentRotated[i], size);
	}
	appState.Scene.previousVertexes = nullptr;
	appState.Scene.previousTexture = nullptr;
	for (auto i = 0; i <= appState.Scene.lastTurbulentRotatedLit; i++)
	{
		appState.Scene.GetStagingBufferSize(appState, d_lists.turbulent_rotated_lit[i], appState.Scene.loadedTurbulentRotatedLit[i], size);
	}
	appState.Scene.previousApverts = nullptr;
	appState.Scene.previousTexture = nullptr;
	for (auto i = 0; i <= appState.Scene.lastAlias; i++)
	{
		appState.Scene.GetStagingBufferSize(appState, d_lists.alias[i], appState.Scene.loadedAlias[i], host_colormap, size);
	}
	appState.Scene.previousApverts = nullptr;
	appState.Scene.previousTexture = nullptr;
	for (auto i = 0; i <= appState.Scene.lastViewmodel; i++)
	{
		appState.Scene.GetStagingBufferSize(appState, d_lists.viewmodels[i], appState.Scene.loadedViewmodels[i], host_colormap, size);
	}
	if (appState.Scene.lastSky >= 0)
	{
		if (sky == nullptr)
		{
			auto mipCount = (int)(std::floor(std::log2(std::max(128, 128)))) + 1;
			sky = new Texture();
			sky->Create(appState, 128, 128, VK_FORMAT_R8_UINT, mipCount, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
		}
		appState.Scene.skySize = 16384;
		size += appState.Scene.skySize;
		appState.Scene.skyCount = d_lists.sky[0].count;
		appState.Scene.firstSkyVertex = d_lists.sky[0].first_vertex;
	}
	appState.Scene.floorVerticesSize = 0;
	if (appState.Mode != AppWorldMode)
	{
		appState.Scene.floorVerticesSize += 3 * 4 * sizeof(float);
	}
	appState.Scene.controllerVerticesSize = 0;
	if (appState.Focused && (key_dest == key_console || key_dest == key_menu || appState.Mode != AppWorldMode))
	{
		if (appState.LeftController.PoseIsValid)
		{
			appState.Scene.controllerVerticesSize += 2 * 8 * 3 * sizeof(float);
		}
		if (appState.RightController.PoseIsValid)
		{
			appState.Scene.controllerVerticesSize += 2 * 8 * 3 * sizeof(float);
		}
	}
	appState.Scene.texturedVerticesSize = (d_lists.last_textured_vertex + 1) * sizeof(float);
	appState.Scene.particlePositionsSize = (d_lists.last_particle_position + 1) * sizeof(float);
	appState.Scene.coloredVerticesSize = (d_lists.last_colored_vertex + 1) * sizeof(float);
	appState.Scene.verticesSize = appState.Scene.floorVerticesSize + appState.Scene.controllerVerticesSize + appState.Scene.texturedVerticesSize + appState.Scene.particlePositionsSize + appState.Scene.coloredVerticesSize;
	if (appState.Scene.verticesSize > 0)
	{
		vertices = cachedVertices.GetVertexBuffer(appState, appState.Scene.verticesSize);
	}
	size += appState.Scene.verticesSize;
	appState.Scene.floorAttributesSize = 0;
	if (appState.Mode != AppWorldMode)
	{
		appState.Scene.floorAttributesSize += 2 * 4 * sizeof(float);
	}
	appState.Scene.controllerAttributesSize = 0;
	if (appState.Scene.controllerVerticesSize > 0)
	{
		if (appState.LeftController.PoseIsValid)
		{
			appState.Scene.controllerAttributesSize += 2 * 8 * 2 * sizeof(float);
		}
		if (appState.RightController.PoseIsValid)
		{
			appState.Scene.controllerAttributesSize += 2 * 8 * 2 * sizeof(float);
		}
	}
	appState.Scene.texturedAttributesSize = (d_lists.last_textured_attribute + 1) * sizeof(float);
	appState.Scene.colormappedLightsSize = (d_lists.last_colormapped_attribute + 1) * sizeof(float);
	appState.Scene.attributesSize = appState.Scene.floorAttributesSize + appState.Scene.controllerAttributesSize + appState.Scene.texturedAttributesSize + appState.Scene.colormappedLightsSize;
	if (appState.Scene.attributesSize > 0)
	{
		attributes = cachedAttributes.GetVertexBuffer(appState, appState.Scene.attributesSize);
	}
	size += appState.Scene.attributesSize;
	appState.Scene.particleColorsSize = (d_lists.last_particle_color + 1) * sizeof(float);
	appState.Scene.coloredColorsSize = (d_lists.last_colored_color + 1) * sizeof(float);
	appState.Scene.colorsSize = appState.Scene.particleColorsSize + appState.Scene.coloredColorsSize;
	if (appState.Scene.colorsSize > 0)
	{
		colors = cachedColors.GetVertexBuffer(appState, appState.Scene.colorsSize);
	}
	size += appState.Scene.colorsSize;
	appState.Scene.floorIndicesSize = 0;
	if (appState.Mode != AppWorldMode)
	{
		appState.Scene.floorIndicesSize = 6;
	}
	appState.Scene.controllerIndicesSize = 0;
	if (appState.Scene.controllerVerticesSize > 0)
	{
		if (appState.LeftController.PoseIsValid)
		{
			appState.Scene.controllerIndicesSize += 2 * 36;
		}
		if (appState.RightController.PoseIsValid)
		{
			appState.Scene.controllerIndicesSize += 2 * 36;
		}
	}
	appState.Scene.coloredIndices8Size = appState.Scene.lastColoredIndex8 + 1;
	appState.Scene.coloredIndices16Size = (appState.Scene.lastColoredIndex16 + 1) * sizeof(uint16_t);
	if (appState.IndexTypeUInt8Enabled)
	{
		appState.Scene.indices8Size = appState.Scene.floorIndicesSize + appState.Scene.controllerIndicesSize + appState.Scene.coloredIndices8Size;
		appState.Scene.indices16Size = appState.Scene.coloredIndices16Size;
	}
	else
	{
		appState.Scene.floorIndicesSize *= sizeof(uint16_t);
		appState.Scene.controllerIndicesSize *= sizeof(uint16_t);
		appState.Scene.indices8Size = 0;
		appState.Scene.indices16Size = appState.Scene.floorIndicesSize + appState.Scene.controllerIndicesSize + appState.Scene.coloredIndices8Size * sizeof(uint16_t) + appState.Scene.coloredIndices16Size;
	}
	if (appState.Scene.indices8Size > 0)
	{
		indices8 = cachedIndices8.GetIndexBuffer(appState, appState.Scene.indices8Size);
	}
	size += appState.Scene.indices8Size;
	if (appState.Scene.indices16Size > 0)
	{
		indices16 = cachedIndices16.GetIndexBuffer(appState, appState.Scene.indices16Size);
	}
	size += appState.Scene.indices16Size;
	appState.Scene.indices32Size = (appState.Scene.lastColoredIndex32 + 1) * sizeof(uint32_t);
	if (appState.Scene.indices32Size > 0)
	{
		indices32 = cachedIndices32.GetIndexBuffer(appState, appState.Scene.indices32Size);
	}
	size += appState.Scene.indices32Size;

	// Add extra space (and also realign to a 4-byte boundary), due to potential alignment issues between 8, 16 and 32-bit index data:
	if (size > 0)
	{
		size += 16;
		while (size % 4 != 0)
		{
			size++;
		}
	}

	return size;
}

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

void PerImage::LoadStagingBuffer(AppState& appState, Buffer* stagingBuffer)
{
	VkDeviceSize offset = 0;
	static const size_t matricesSize = 2 * sizeof(XrMatrix4x4f);
	memcpy(stagingBuffer->mapped, appState.ViewMatrices.data(), matricesSize);
	offset += matricesSize;
	memcpy(((unsigned char*)stagingBuffer->mapped) + offset, appState.ProjectionMatrices.data(), matricesSize);
	offset += matricesSize;
	auto target = ((float*)stagingBuffer->mapped) + offset / sizeof(float);
	*target++ = appState.Scale;
	*target++ = 0;
	*target++ = 0;
	*target++ = 0;
	*target++ = 0;
	*target++ = appState.Scale;
	*target++ = 0;
	*target++ = 0;
	*target++ = 0;
	*target++ = 0;
	*target++ = appState.Scale;
	*target++ = 0;
	*target++ = -r_refdef.vieworg[0] * appState.Scale;
	*target++ = -r_refdef.vieworg[2] * appState.Scale;
	*target++ = r_refdef.vieworg[1] * appState.Scale;
	*target++ = 1;
	offset += sizeof(XrMatrix4x4f);
	auto loadedBuffer = appState.Scene.buffers.firstVertices;
	while (loadedBuffer != nullptr)
	{
		memcpy(((unsigned char*)stagingBuffer->mapped) + offset, loadedBuffer->source, loadedBuffer->size);
		offset += loadedBuffer->size;
		loadedBuffer = loadedBuffer->next;
	}
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
	auto loadedTexturePositionBuffer = appState.Scene.buffers.firstSurfaceTexturePositions;
	while (loadedTexturePositionBuffer != nullptr)
	{
		auto source = (msurface_t*)loadedTexturePositionBuffer->source;
		auto target = (float*)(((unsigned char*)stagingBuffer->mapped) + offset);
		*target++ = source->texinfo->vecs[0][0];
		*target++ = source->texinfo->vecs[0][1];
		*target++ = source->texinfo->vecs[0][2];
		*target++ = source->texinfo->vecs[0][3];
		*target++ = source->texinfo->vecs[1][0];
		*target++ = source->texinfo->vecs[1][1];
		*target++ = source->texinfo->vecs[1][2];
		*target++ = source->texinfo->vecs[1][3];
		*target++ = source->texturemins[0];
		*target++ = source->texturemins[1];
		*target++ = source->extents[0];
		*target++ = source->extents[1];
		*target++ = source->texinfo->texture->width;
		*target++ = source->texinfo->texture->height;
		offset += loadedTexturePositionBuffer->size;
		loadedTexturePositionBuffer = loadedTexturePositionBuffer->next;
	}
	loadedTexturePositionBuffer = appState.Scene.buffers.firstTurbulentTexturePositions;
	while (loadedTexturePositionBuffer != nullptr)
	{
		auto source = (msurface_t*)loadedTexturePositionBuffer->source;
		auto target = (float*)(((unsigned char*)stagingBuffer->mapped) + offset);
		*target++ = source->texinfo->vecs[0][0];
		*target++ = source->texinfo->vecs[0][1];
		*target++ = source->texinfo->vecs[0][2];
		*target++ = source->texinfo->vecs[0][3];
		*target++ = source->texinfo->vecs[1][0];
		*target++ = source->texinfo->vecs[1][1];
		*target++ = source->texinfo->vecs[1][2];
		*target++ = source->texinfo->vecs[1][3];
		*target++ = source->texinfo->texture->width;
		*target++ = source->texinfo->texture->height;
		offset += loadedTexturePositionBuffer->size;
		loadedTexturePositionBuffer = loadedTexturePositionBuffer->next;
	}
	loadedBuffer = appState.Scene.buffers.firstAliasVertices;
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
			auto target = ((float*)stagingBuffer->mapped) + offset / sizeof(float);
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
			auto target = ((float*)stagingBuffer->mapped) + offset / sizeof(float);
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
		auto target = ((float*)stagingBuffer->mapped) + offset / sizeof(float);
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
		auto target = ((float*)stagingBuffer->mapped) + offset / sizeof(float);
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
				*target++ = 2;
				*target++ = 2;
				*target++ = 3;
				*target++ = 0;
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
	else
	{
		if (appState.Scene.indices16Size > 0)
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
		}
	}
	while (offset % 4 != 0)
	{
		offset++;
	}
	memcpy(((unsigned char*)stagingBuffer->mapped) + offset, d_lists.colored_indices32.data(), appState.Scene.indices32Size);
	offset += appState.Scene.indices32Size;
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

void PerImage::FillColormapTextures(AppState& appState, LoadedAlias& loaded)
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

void PerImage::FillTexturePositionsFromStagingBuffer(AppState& appState, Buffer* stagingBuffer, LoadedSharedMemoryTexturePositionsBuffer* first, VkBufferCopy& bufferCopy, SharedMemoryBuffer*& previousBuffer) const
{
	auto loaded = first;
	auto delayed = false;
	while (loaded != nullptr)
	{
		if (delayed)
		{
			if (previousBuffer == loaded->texturePositions.buffer && bufferCopy.dstOffset + bufferCopy.size == loaded->texturePositions.offset)
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
			bufferCopy.dstOffset = loaded->texturePositions.offset;
			bufferCopy.size = loaded->size;
			delayed = true;
		}
		if (previousBuffer != loaded->texturePositions.buffer)
		{
			appState.Scene.AddToBufferBarrier(loaded->texturePositions.buffer->buffer);
			previousBuffer = loaded->texturePositions.buffer;
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

void PerImage::FillFromStagingBuffer(AppState& appState, Buffer* stagingBuffer)
{
	appState.Scene.stagingBuffer.lastBarrier = -1;
	appState.Scene.stagingBuffer.descriptorSetsInUse.clear();

	VkBufferCopy bufferCopy { };
	bufferCopy.size = matrices.size;
	vkCmdCopyBuffer(commandBuffer, stagingBuffer->buffer, matrices.buffer, 1, &bufferCopy);

	VkBufferMemoryBarrier barrier { VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER };
	barrier.buffer = matrices.buffer;
	barrier.size = VK_WHOLE_SIZE;
	barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	barrier.dstAccessMask = VK_ACCESS_UNIFORM_READ_BIT;
	vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_VERTEX_SHADER_BIT, 0, 0, nullptr, 1, &barrier, 0, nullptr);

	bufferCopy.srcOffset += matrices.size;
	auto loadedBuffer = appState.Scene.buffers.firstVertices;
	while (loadedBuffer != nullptr)
	{
		bufferCopy.size = loadedBuffer->size;
		vkCmdCopyBuffer(commandBuffer, stagingBuffer->buffer, loadedBuffer->buffer->buffer, 1, &bufferCopy);
		bufferCopy.srcOffset += bufferCopy.size;
		appState.Scene.AddToBufferBarrier(loadedBuffer->buffer->buffer);
		loadedBuffer = loadedBuffer->next;
	}

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

	SharedMemoryBuffer* previousSharedMemoryBuffer = nullptr;
	FillTexturePositionsFromStagingBuffer(appState, stagingBuffer, appState.Scene.buffers.firstSurfaceTexturePositions, bufferCopy, previousSharedMemoryBuffer);
	FillTexturePositionsFromStagingBuffer(appState, stagingBuffer, appState.Scene.buffers.firstTurbulentTexturePositions, bufferCopy, previousSharedMemoryBuffer);

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
	auto loadedTexture = appState.Scene.textures.first;
	while (loadedTexture != nullptr)
	{
		loadedTexture->texture->FillMipmapped(appState, appState.Scene.stagingBuffer);
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
		sky->FillMipmapped(appState, appState.Scene.stagingBuffer);
		appState.Scene.stagingBuffer.offset += appState.Scene.skySize;
	}

	if (appState.Scene.stagingBuffer.lastBarrier >= 0)
	{
		vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, appState.Scene.stagingBuffer.lastBarrier + 1, appState.Scene.stagingBuffer.imageBarriers.data());
	}
}

void PerImage::SetPushConstants(const LoadedSurfaceRotated& loaded, SurfaceRotatedPushConstants& pushConstants)
{
	pushConstants.lightmapIndex = loaded.lightmap.lightmap->allocatedIndex;
	pushConstants.originX = loaded.originX;
	pushConstants.originY = loaded.originY;
	pushConstants.originZ = loaded.originZ;
	pushConstants.yaw = loaded.yaw * M_PI / 180;
	pushConstants.pitch = loaded.pitch * M_PI / 180;
	pushConstants.roll = -loaded.roll * M_PI / 180;
}

void PerImage::SetPushConstants(const LoadedAlias& loaded, float pushConstants[])
{
	pushConstants[0] = loaded.transform[0][0];
	pushConstants[1] = loaded.transform[2][0];
	pushConstants[2] = -loaded.transform[1][0];
	pushConstants[4] = loaded.transform[0][2];
	pushConstants[5] = loaded.transform[2][2];
	pushConstants[6] = -loaded.transform[1][2];
	pushConstants[8] = -loaded.transform[0][1];
	pushConstants[9] = -loaded.transform[2][1];
	pushConstants[10] = loaded.transform[1][1];
	pushConstants[12] = loaded.transform[0][3];
	pushConstants[13] = loaded.transform[2][3];
	pushConstants[14] = -loaded.transform[1][3];
}

void PerImage::Render(AppState& appState)
{
	if (appState.Scene.lastSurface < 0 &&
		appState.Scene.lastSurfaceRotated < 0 &&
		appState.Scene.lastFence < 0 &&
		appState.Scene.lastFenceRotated < 0 &&
		appState.Scene.lastTurbulent < 0 &&
		appState.Scene.lastTurbulentRotated < 0 &&
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
		float pushConstants[24];
		if (appState.Scene.lastSky >= 0)
		{
			vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.sky.pipeline);
			vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertices->buffer, &texturedVertexBase);
			vkCmdBindVertexBuffers(commandBuffer, 1, 1, &attributes->buffer, &texturedAttributeBase);
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
			vkCmdDraw(commandBuffer, appState.Scene.skyCount, 1, appState.Scene.firstSkyVertex, 0);
		}
		if (appState.Scene.lastSurface >= 0)
		{
			vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.surfaces.pipeline);
			vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.surfaces.pipelineLayout, 0, 1, &sceneMatricesAndColormapResources.descriptorSet, 0, nullptr);
			for (auto& entry : appState.Scene.sortedSurfaces)
			{
				for (auto& subEntry : entry.lightmapDescriptorSets)
				{
					subEntry.entries.clear();
				}
			}
			for (auto i = 0; i <= appState.Scene.lastSurface; i++)
			{
				auto& loaded = appState.Scene.loadedSurfaces[i];
				auto textureDescriptorSet = loaded.texture.texture->descriptorSet;
				auto lightmapDescriptorSet = loaded.lightmap.lightmap->texture->descriptorSet;
				auto entryFound = false;
				for (auto& entry : appState.Scene.sortedSurfaces)
				{
					if (entry.textureDescriptorSet == textureDescriptorSet)
					{
						entryFound = true;
						auto subEntryFound = false;
						for (auto& subEntry : entry.lightmapDescriptorSets)
						{
							if (subEntry.lightmapDescriptorSet == lightmapDescriptorSet)
							{
								subEntryFound = true;
								subEntry.entries.push_back(i);
								break;
							}
						}
						if (!subEntryFound)
						{
							entry.lightmapDescriptorSets.push_back({ lightmapDescriptorSet });
							entry.lightmapDescriptorSets.back().entries.push_back(i);
						}
						break;
					}
				}
				if (!entryFound)
				{
					appState.Scene.sortedSurfaces.push_back({ textureDescriptorSet });
					appState.Scene.sortedSurfaces.back().lightmapDescriptorSets.push_back({ lightmapDescriptorSet });
					appState.Scene.sortedSurfaces.back().lightmapDescriptorSets.back().entries.push_back(i);
				}
				for (auto entry = appState.Scene.sortedSurfaces.begin(); entry != appState.Scene.sortedSurfaces.end(); )
				{
					for (auto subEntry = entry->lightmapDescriptorSets.begin(); subEntry != entry->lightmapDescriptorSets.end(); )
					{
						if (subEntry->entries.size() == 0)
						{
							subEntry = entry->lightmapDescriptorSets.erase(subEntry);
						}
						else
						{
							subEntry++;
						}
					}
					if (entry->lightmapDescriptorSets.size() == 0)
					{
						entry = appState.Scene.sortedSurfaces.erase(entry);
					}
					else
					{
						entry++;
					}
				}
			}
			SharedMemoryBuffer* previousVertices = nullptr;
			SharedMemoryBuffer* previousTexturePositions = nullptr;
			SharedMemoryBuffer* previousIndices = nullptr;
			for (auto& entry : appState.Scene.sortedSurfaces)
			{
				vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.surfaces.pipelineLayout, 2, 1, &entry.textureDescriptorSet, 0, nullptr);
				for (auto& subEntry : entry.lightmapDescriptorSets)
				{
					vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.surfaces.pipelineLayout, 1, 1, &subEntry.lightmapDescriptorSet, 0, nullptr);
					for (auto i : subEntry.entries)
					{
						auto& loaded = appState.Scene.loadedSurfaces[i];
						auto vertices = loaded.vertices.buffer;
						if (previousVertices != vertices)
						{
							vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertices->buffer, &appState.NoOffset);
							previousVertices = vertices;
						}
						auto texturePositions = loaded.texturePositions.texturePositions.buffer;
						if (previousTexturePositions != texturePositions)
						{
							vkCmdBindVertexBuffers(commandBuffer, 1, 1, &texturePositions->buffer, &appState.NoOffset);
							previousTexturePositions = texturePositions;
						}
						uint32_t lightmapIndex = loaded.lightmap.lightmap->allocatedIndex;
						vkCmdPushConstants(commandBuffer, appState.Scene.surfaces.pipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(uint32_t), &lightmapIndex);
						auto indices = loaded.indices.indices.buffer;
						if (previousIndices != indices)
						{
							vkCmdBindIndexBuffer(commandBuffer, indices->buffer, 0, loaded.indices.indices.indexType);
							previousIndices = indices;
						}
						vkCmdDrawIndexed(commandBuffer, loaded.count, 1, loaded.indices.indices.firstIndex, 0, loaded.texturePositions.texturePositions.firstInstance);
					}
				}
			}
		}
		SurfaceRotatedPushConstants surfaceRotatedPushConstants;
		if (appState.Scene.lastSurfaceRotated >= 0)
		{
			vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.surfacesRotated.pipeline);
			vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.surfacesRotated.pipelineLayout, 0, 1, &sceneMatricesAndColormapResources.descriptorSet, 0, nullptr);
			SharedMemoryBuffer* previousVertices = nullptr;
			SharedMemoryBuffer* previousTexturePositions = nullptr;
			VkDescriptorSet previousLightmapDescriptorSet = VK_NULL_HANDLE;
			VkDescriptorSet previousTextureDescriptorSet = VK_NULL_HANDLE;
			SharedMemoryBuffer* previousIndices = nullptr;
			for (auto i = 0; i <= appState.Scene.lastSurfaceRotated; i++)
			{
				auto& loaded = appState.Scene.loadedSurfacesRotated[i];
				auto vertices = loaded.vertices.buffer;
				if (previousVertices != vertices)
				{
					vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertices->buffer, &appState.NoOffset);
					previousVertices = vertices;
				}
				auto texturePositions = loaded.texturePositions.texturePositions.buffer;
				if (previousTexturePositions != texturePositions)
				{
					vkCmdBindVertexBuffers(commandBuffer, 1, 1, &texturePositions->buffer, &appState.NoOffset);
					previousTexturePositions = texturePositions;
				}
				auto lightmapDescriptorSet = loaded.lightmap.lightmap->texture->descriptorSet;
				if (previousLightmapDescriptorSet != lightmapDescriptorSet)
				{
					vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.surfacesRotated.pipelineLayout, 1, 1, &lightmapDescriptorSet, 0, nullptr);
					previousLightmapDescriptorSet = lightmapDescriptorSet;
				}
				auto textureDescriptorSet = loaded.texture.texture->descriptorSet;
				if (previousTextureDescriptorSet != textureDescriptorSet)
				{
					vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.surfacesRotated.pipelineLayout, 2, 1, &textureDescriptorSet, 0, nullptr);
					previousTextureDescriptorSet = textureDescriptorSet;
				}
				SetPushConstants(loaded, surfaceRotatedPushConstants);
				vkCmdPushConstants(commandBuffer, appState.Scene.surfacesRotated.pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(uint32_t) + 6 * sizeof(float), &surfaceRotatedPushConstants);
				auto indices = loaded.indices.indices.buffer;
				if (previousIndices != indices)
				{
					vkCmdBindIndexBuffer(commandBuffer, indices->buffer, 0, loaded.indices.indices.indexType);
					previousIndices = indices;
				}
				vkCmdDrawIndexed(commandBuffer, loaded.count, 1, loaded.indices.indices.firstIndex, 0, loaded.texturePositions.texturePositions.firstInstance);
			}
		}
		if (appState.Scene.lastFence >= 0)
		{
			vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.fences.pipeline);
			vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.fences.pipelineLayout, 0, 1, &sceneMatricesAndColormapResources.descriptorSet, 0, nullptr);
			SharedMemoryBuffer* previousVertices = nullptr;
			SharedMemoryBuffer* previousTexturePositions = nullptr;
			VkDescriptorSet previousLightmapDescriptorSet = VK_NULL_HANDLE;
			VkDescriptorSet previousTextureDescriptorSet = VK_NULL_HANDLE;
			SharedMemoryBuffer* previousIndices = nullptr;
			for (auto i = 0; i <= appState.Scene.lastFence; i++)
			{
				auto& loaded = appState.Scene.loadedFences[i];
				auto vertices = loaded.vertices.buffer;
				if (previousVertices != vertices)
				{
					vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertices->buffer, &appState.NoOffset);
					previousVertices = vertices;
				}
				auto texturePositions = loaded.texturePositions.texturePositions.buffer;
				if (previousTexturePositions != texturePositions)
				{
					vkCmdBindVertexBuffers(commandBuffer, 1, 1, &texturePositions->buffer, &appState.NoOffset);
					previousTexturePositions = texturePositions;
				}
				auto lightmap = loaded.lightmap.lightmap;
				auto lightmapDescriptorSet = lightmap->texture->descriptorSet;
				if (previousLightmapDescriptorSet != lightmapDescriptorSet)
				{
					vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.fences.pipelineLayout, 1, 1, &lightmapDescriptorSet, 0, nullptr);
					previousLightmapDescriptorSet = lightmapDescriptorSet;
				}
				auto textureDescriptorSet = loaded.texture.texture->descriptorSet;
				if (previousTextureDescriptorSet != textureDescriptorSet)
				{
					vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.fences.pipelineLayout, 2, 1, &textureDescriptorSet, 0, nullptr);
					previousTextureDescriptorSet = textureDescriptorSet;
				}
				uint32_t lightmapIndex = loaded.lightmap.lightmap->allocatedIndex;
				vkCmdPushConstants(commandBuffer, appState.Scene.fences.pipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(uint32_t), &lightmapIndex);
				auto indices = loaded.indices.indices.buffer;
				if (previousIndices != indices)
				{
					vkCmdBindIndexBuffer(commandBuffer, indices->buffer, 0, loaded.indices.indices.indexType);
					previousIndices = indices;
				}
				vkCmdDrawIndexed(commandBuffer, loaded.count, 1, loaded.indices.indices.firstIndex, 0, loaded.texturePositions.texturePositions.firstInstance);
			}
		}
		if (appState.Scene.lastFenceRotated >= 0)
		{
			vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.fencesRotated.pipeline);
			vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.fencesRotated.pipelineLayout, 0, 1, &sceneMatricesAndColormapResources.descriptorSet, 0, nullptr);
			SharedMemoryBuffer* previousVertices = nullptr;
			SharedMemoryBuffer* previousTexturePositions = nullptr;
			VkDescriptorSet previousLightmapDescriptorSet = VK_NULL_HANDLE;
			VkDescriptorSet previousTextureDescriptorSet = VK_NULL_HANDLE;
			SharedMemoryBuffer* previousIndices = nullptr;
			for (auto i = 0; i <= appState.Scene.lastFenceRotated; i++)
			{
				auto& loaded = appState.Scene.loadedFencesRotated[i];
				auto vertices = loaded.vertices.buffer;
				if (previousVertices != vertices)
				{
					vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertices->buffer, &appState.NoOffset);
					previousVertices = vertices;
				}
				auto texturePositions = loaded.texturePositions.texturePositions.buffer;
				if (previousTexturePositions != texturePositions)
				{
					vkCmdBindVertexBuffers(commandBuffer, 1, 1, &texturePositions->buffer, &appState.NoOffset);
					previousTexturePositions = texturePositions;
				}
				auto lightmap = loaded.lightmap.lightmap;
				auto lightmapDescriptorSet = lightmap->texture->descriptorSet;
				if (previousLightmapDescriptorSet != lightmapDescriptorSet)
				{
					vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.fencesRotated.pipelineLayout, 1, 1, &lightmapDescriptorSet, 0, nullptr);
					previousLightmapDescriptorSet = lightmapDescriptorSet;
				}
				auto textureDescriptorSet = loaded.texture.texture->descriptorSet;
				if (previousTextureDescriptorSet != textureDescriptorSet)
				{
					vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.fencesRotated.pipelineLayout, 2, 1, &textureDescriptorSet, 0, nullptr);
					previousTextureDescriptorSet = textureDescriptorSet;
				}
				SetPushConstants(loaded, surfaceRotatedPushConstants);
				vkCmdPushConstants(commandBuffer, appState.Scene.fencesRotated.pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(uint32_t) + 6 * sizeof(float), &surfaceRotatedPushConstants);
				auto indices = loaded.indices.indices.buffer;
				if (previousIndices != indices)
				{
					vkCmdBindIndexBuffer(commandBuffer, indices->buffer, 0, loaded.indices.indices.indexType);
					previousIndices = indices;
				}
				vkCmdDrawIndexed(commandBuffer, loaded.count, 1, loaded.indices.indices.firstIndex, 0, loaded.texturePositions.texturePositions.firstInstance);
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
		if (appState.Scene.lastTurbulent >= 0)
		{
			vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.turbulent.pipeline);
			vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.turbulent.pipelineLayout, 0, 1, &sceneMatricesAndPaletteResources.descriptorSet, 0, nullptr);
			auto time = (float)cl.time;
			SharedMemoryBuffer* previousVertices = nullptr;
			SharedMemoryBuffer* previousTexturePositions = nullptr;
			VkDescriptorSet previousTextureDescriptorSet = VK_NULL_HANDLE;
			SharedMemoryBuffer* previousIndices = nullptr;
			for (auto i = 0; i <= appState.Scene.lastTurbulent; i++)
			{
				auto& loaded = appState.Scene.loadedTurbulent[i];
				auto vertices = loaded.vertices.buffer;
				if (previousVertices != vertices)
				{
					vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertices->buffer, &appState.NoOffset);
					previousVertices = vertices;
				}
				auto texturePositions = loaded.texturePositions.texturePositions.buffer;
				if (previousTexturePositions != texturePositions)
				{
					vkCmdBindVertexBuffers(commandBuffer, 1, 1, &texturePositions->buffer, &appState.NoOffset);
					previousTexturePositions = texturePositions;
				}
				auto textureDescriptorSet = loaded.texture.texture->descriptorSet;
				if (previousTextureDescriptorSet != textureDescriptorSet)
				{
					vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.turbulent.pipelineLayout, 1, 1, &textureDescriptorSet, 0, nullptr);
					previousTextureDescriptorSet = textureDescriptorSet;
				}
				vkCmdPushConstants(commandBuffer, appState.Scene.turbulent.pipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(float), &time);
				auto indices = loaded.indices.indices.buffer;
				if (previousIndices != indices)
				{
					vkCmdBindIndexBuffer(commandBuffer, indices->buffer, 0, loaded.indices.indices.indexType);
					previousIndices = indices;
				}
				vkCmdDrawIndexed(commandBuffer, loaded.count, 1, loaded.indices.indices.firstIndex, 0, loaded.texturePositions.texturePositions.firstInstance);
			}
		}
		if (appState.Scene.lastTurbulentLit >= 0)
		{
			TurbulentLitPushConstants pushConstants;
			vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.turbulentLit.pipeline);
			vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.turbulentLit.pipelineLayout, 0, 1, &sceneMatricesAndColormapResources.descriptorSet, 0, nullptr);
			pushConstants.time = (float)cl.time;
			SharedMemoryBuffer* previousVertices = nullptr;
			SharedMemoryBuffer* previousTexturePositions = nullptr;
			VkDescriptorSet previousLightmapDescriptorSet = VK_NULL_HANDLE;
			VkDescriptorSet previousTextureDescriptorSet = VK_NULL_HANDLE;
			SharedMemoryBuffer* previousIndices = nullptr;
			for (auto i = 0; i <= appState.Scene.lastTurbulentLit; i++)
			{
				auto& loaded = appState.Scene.loadedTurbulentLit[i];
				auto vertices = loaded.vertices.buffer;
				if (previousVertices != vertices)
				{
					vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertices->buffer, &appState.NoOffset);
					previousVertices = vertices;
				}
				auto texturePositions = loaded.texturePositions.texturePositions.buffer;
				if (previousTexturePositions != texturePositions)
				{
					vkCmdBindVertexBuffers(commandBuffer, 1, 1, &texturePositions->buffer, &appState.NoOffset);
					previousTexturePositions = texturePositions;
				}
				auto lightmapDescriptorSet = loaded.lightmap.lightmap->texture->descriptorSet;
				if (previousLightmapDescriptorSet != lightmapDescriptorSet)
				{
					vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.turbulentLit.pipelineLayout, 1, 1, &lightmapDescriptorSet, 0, nullptr);
					previousLightmapDescriptorSet = lightmapDescriptorSet;
				}
				auto textureDescriptorSet = loaded.texture.texture->descriptorSet;
				if (previousTextureDescriptorSet != textureDescriptorSet)
				{
					vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.turbulentLit.pipelineLayout, 2, 1, &textureDescriptorSet, 0, nullptr);
					previousTextureDescriptorSet = textureDescriptorSet;
				}
				pushConstants.lightmapIndex = loaded.lightmap.lightmap->allocatedIndex;
				vkCmdPushConstants(commandBuffer, appState.Scene.turbulentLit.pipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(float) + sizeof(uint32_t), &pushConstants);
				auto indices = loaded.indices.indices.buffer;
				if (previousIndices != indices)
				{
					vkCmdBindIndexBuffer(commandBuffer, indices->buffer, 0, loaded.indices.indices.indexType);
					previousIndices = indices;
				}
				vkCmdDrawIndexed(commandBuffer, loaded.count, 1, loaded.indices.indices.firstIndex, 0, loaded.texturePositions.texturePositions.firstInstance);
			}
		}
		if (appState.Scene.lastTurbulentRotated >= 0)
		{
			vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.turbulentRotated.pipeline);
			vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.turbulentRotated.pipelineLayout, 0, 1, &sceneMatricesAndPaletteResources.descriptorSet, 0, nullptr);
			pushConstants[6] = (float)cl.time;
			SharedMemoryBuffer* previousVertices = nullptr;
			SharedMemoryBuffer* previousTexturePositions = nullptr;
			VkDescriptorSet previousTextureDescriptorSet = VK_NULL_HANDLE;
			SharedMemoryBuffer* previousIndices = nullptr;
			for (auto i = 0; i <= appState.Scene.lastTurbulentRotated; i++)
			{
				auto& loaded = appState.Scene.loadedTurbulentRotated[i];
				auto vertices = loaded.vertices.buffer;
				if (previousVertices != vertices)
				{
					vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertices->buffer, &appState.NoOffset);
					previousVertices = vertices;
				}
				auto texturePositions = loaded.texturePositions.texturePositions.buffer;
				if (previousTexturePositions != texturePositions)
				{
					vkCmdBindVertexBuffers(commandBuffer, 1, 1, &texturePositions->buffer, &appState.NoOffset);
					previousTexturePositions = texturePositions;
				}
				auto textureDescriptorSet = loaded.texture.texture->descriptorSet;
				if (previousTextureDescriptorSet != textureDescriptorSet)
				{
					vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.turbulentRotated.pipelineLayout, 1, 1, &textureDescriptorSet, 0, nullptr);
					previousTextureDescriptorSet = textureDescriptorSet;
				}
				pushConstants[0] = loaded.originX;
				pushConstants[1] = loaded.originY;
				pushConstants[2] = loaded.originZ;
				pushConstants[3] = loaded.yaw * M_PI / 180;
				pushConstants[4] = loaded.pitch * M_PI / 180;
				pushConstants[5] = -loaded.roll * M_PI / 180;
				vkCmdPushConstants(commandBuffer, appState.Scene.turbulentRotated.pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, 7 * sizeof(float), pushConstants);
				auto indices = loaded.indices.indices.buffer;
				if (previousIndices != indices)
				{
					vkCmdBindIndexBuffer(commandBuffer, indices->buffer, 0, loaded.indices.indices.indexType);
					previousIndices = indices;
				}
				vkCmdDrawIndexed(commandBuffer, loaded.count, 1, loaded.indices.indices.firstIndex, 0, loaded.texturePositions.texturePositions.firstInstance);
			}
		}
		if (appState.Scene.lastTurbulentRotatedLit >= 0)
		{
			TurbulentRotatedLitPushConstants turbulentRotatedLitPushConstants;
			vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.turbulentRotatedLit.pipeline);
			vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.turbulentRotatedLit.pipelineLayout, 0, 1, &sceneMatricesAndPaletteResources.descriptorSet, 0, nullptr);
			turbulentRotatedLitPushConstants.time = (float)cl.time;
			SharedMemoryBuffer* previousVertices = nullptr;
			SharedMemoryBuffer* previousTexturePositions = nullptr;
			VkDescriptorSet previousLightmapDescriptorSet = VK_NULL_HANDLE;
			VkDescriptorSet previousTextureDescriptorSet = VK_NULL_HANDLE;
			SharedMemoryBuffer* previousIndices = nullptr;
			for (auto i = 0; i <= appState.Scene.lastTurbulentRotatedLit; i++)
			{
				auto& loaded = appState.Scene.loadedTurbulentRotatedLit[i];
				auto vertices = loaded.vertices.buffer;
				if (previousVertices != vertices)
				{
					vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertices->buffer, &appState.NoOffset);
					previousVertices = vertices;
				}
				auto texturePositions = loaded.texturePositions.texturePositions.buffer;
				if (previousTexturePositions != texturePositions)
				{
					vkCmdBindVertexBuffers(commandBuffer, 1, 1, &texturePositions->buffer, &appState.NoOffset);
					previousTexturePositions = texturePositions;
				}
				auto lightmapDescriptorSet = loaded.lightmap.lightmap->texture->descriptorSet;
				if (previousLightmapDescriptorSet != lightmapDescriptorSet)
				{
					vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.turbulentLit.pipelineLayout, 1, 1, &lightmapDescriptorSet, 0, nullptr);
					previousLightmapDescriptorSet = lightmapDescriptorSet;
				}
				auto textureDescriptorSet = loaded.texture.texture->descriptorSet;
				if (previousTextureDescriptorSet != textureDescriptorSet)
				{
					vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.turbulentRotatedLit.pipelineLayout, 2, 1, &textureDescriptorSet, 0, nullptr);
					previousTextureDescriptorSet = textureDescriptorSet;
				}
				turbulentRotatedLitPushConstants.lightmapIndex = loaded.lightmap.lightmap->allocatedIndex;
				turbulentRotatedLitPushConstants.originX = loaded.originX;
				turbulentRotatedLitPushConstants.originY = loaded.originY;
				turbulentRotatedLitPushConstants.originZ = loaded.originZ;
				turbulentRotatedLitPushConstants.yaw = loaded.yaw * M_PI / 180;
				turbulentRotatedLitPushConstants.pitch = loaded.pitch * M_PI / 180;
				turbulentRotatedLitPushConstants.roll = -loaded.roll * M_PI / 180;
				vkCmdPushConstants(commandBuffer, appState.Scene.turbulentRotated.pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(uint32_t) + 7 * sizeof(float), pushConstants);
				auto indices = loaded.indices.indices.buffer;
				if (previousIndices != indices)
				{
					vkCmdBindIndexBuffer(commandBuffer, indices->buffer, 0, loaded.indices.indices.indexType);
					previousIndices = indices;
				}
				vkCmdDrawIndexed(commandBuffer, loaded.count, 1, loaded.indices.indices.firstIndex, 0, loaded.texturePositions.texturePositions.firstInstance);
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
			if (appState.NearViewmodel)
			{
				pushConstants[16] = appState.ViewmodelForwardX;
				pushConstants[17] = appState.ViewmodelForwardY;
				pushConstants[18] = appState.ViewmodelForwardZ;
				pushConstants[19] = 0;
				pushConstants[20] = 1;
				pushConstants[21] = 1;
				pushConstants[22] = 1;
				pushConstants[23] = 1;
			}
			else
			{
				pushConstants[16] = 1 / appState.Scale;
				pushConstants[17] = 0;
				pushConstants[18] = 0;
				pushConstants[19] = 8;
				pushConstants[20] = 1;
				pushConstants[21] = 0;
				pushConstants[22] = 0;
				pushConstants[23] = 0.7 + 0.3 * sin(cl.time * M_PI);
			}
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
				vkCmdPushConstants(commandBuffer, appState.Scene.viewmodel.pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, 24 * sizeof(float), pushConstants);
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
			pushConstants[0] = appState.Scene.vright0;
			pushConstants[1] = appState.Scene.vright2;
			pushConstants[2] = -appState.Scene.vright1;
			pushConstants[3] = 0;
			pushConstants[4] = appState.Scene.vup0;
			pushConstants[5] = appState.Scene.vup2;
			pushConstants[6] = -appState.Scene.vup1;
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
		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, appState.Scene.floor.pipelineLayout, 0, 2, descriptorSets, 0, nullptr);
		if (appState.IndexTypeUInt8Enabled)
		{
			vkCmdBindIndexBuffer(commandBuffer, indices8->buffer, 0, VK_INDEX_TYPE_UINT8_EXT);
		}
		else
		{
			vkCmdBindIndexBuffer(commandBuffer, indices16->buffer, 0, VK_INDEX_TYPE_UINT16);
		}
		vkCmdDrawIndexed(commandBuffer, 6, 1, 0, 0, 0);
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

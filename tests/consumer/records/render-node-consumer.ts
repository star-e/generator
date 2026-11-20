import type { Material } from '../../asset/assets';
import { ClearFlagBit, Color, Viewport } from '../../gfx';
import type { Buffer, Sampler, Texture } from '../../gfx';
import type { RenderScene } from '../../render-scene/core/render-scene';
import type { Camera } from '../../render-scene/scene/camera';
import type { Light, Model } from '../../render-scene/scene';
import { BlitType, ClearView, CullingFlags, RenderGraphObjectPool, SceneData } from './render-graph';
import { LightInfo, QueueHint, RenderCommonObjectPool, SceneFlags } from './types';

function check(value: boolean, field: string): void {
    if (!value) throw new Error(field);
}

const pool = new RenderGraphObjectPool(new RenderCommonObjectPool());
const scene = {} as RenderScene;
const camera = {} as Camera;
const light = {} as Light;
const material = {} as Material;
const model = {} as Model;
const color = new Color(1, 2, 3, 4);
const culling = CullingFlags.LIGHT_FRUSTUM | CullingFlags.LIGHT_BOUNDS;
const clear = new ClearView('depth', ClearFlagBit.DEPTH, color);
check(clear.clearColor === color && clear.slotName === 'depth' && clear.clearFlags === ClearFlagBit.DEPTH, 'ClearView constructor');
const lightInfo = new LightInfo(light, 3, true);
const sceneValue = new SceneData(scene, camera, SceneFlags.OPAQUE, lightInfo, culling, light);
check(sceneValue.light === lightInfo && sceneValue.shadingLight === light && sceneValue.cullingFlags === culling, 'SceneData constructor');

const view = pool.createClearView('color', ClearFlagBit.COLOR);
const pooledColor = view.clearColor;
pooledColor.copy(color);
const queue = pool.createRenderQueue(QueueHint.BLEND, 4, 5);
check(queue.viewport === null && queue.phaseID === 4 && queue.passLayoutID === 5 && queue.hint === QueueHint.BLEND, 'RenderQueue arguments');
queue.viewport = new Viewport();
const data = pool.createSceneData(scene, camera, SceneFlags.SHADOW_CASTER, culling, light);
const pooledLight = data.light;
pooledLight.reset(light, 7, true, null);
check(data.scene === scene && data.camera === camera && data.flags === SceneFlags.SHADOW_CASTER &&
    data.cullingFlags === culling && data.shadingLight === light, 'SceneData arguments');
const dispatch = pool.createDispatch(material, 1, 2, 3, 4);
check(dispatch.material === material && dispatch.passID === 1 && dispatch.threadGroupCountX === 2 &&
    dispatch.threadGroupCountY === 3 && dispatch.threadGroupCountZ === 4, 'Dispatch arguments');
const blit = pool.createBlit(material, 8, SceneFlags.BLEND, camera, BlitType.DRAW_3D);
const models: Model[] = blit.models;
models.push(model);
check(blit.camera === camera && blit.material === material && blit.passID === 8 && blit.sceneFlags === SceneFlags.BLEND &&
    blit.blitType === BlitType.DRAW_3D, 'Blit arguments');
const render = pool.createRenderData();
const constants: Map<number, number[]> = render.constants;
const buffers: Map<number, Buffer> = render.buffers;
const textures: Map<number, Texture> = render.textures;
const samplers: Map<number, Sampler> = render.samplers;
constants.set(2, [0, 127, 255]);
buffers.set(3, {} as Buffer);
textures.set(4, {} as Texture);
samplers.set(5, {} as Sampler);
render.custom = 'lighting';

pool.reset();
check(pool.createClearView() === view && view.clearColor === pooledColor && view.clearFlags === ClearFlagBit.ALL &&
    view.slotName === '' && [pooledColor.x, pooledColor.y, pooledColor.z, pooledColor.w].every(v => v === 0), 'ClearView reset');
check(pool.createRenderQueue() === queue && queue.viewport === null && queue.hint === QueueHint.RENDER_OPAQUE &&
    queue.phaseID === 0xFFFFFFFF && queue.passLayoutID === 0xFFFFFFFF, 'RenderQueue reset');
check(pool.createSceneData() === data && data.light === pooledLight && pooledLight.light === null && pooledLight.level === 0 &&
    !pooledLight.culledByLight && data.scene === null && data.camera === null && data.shadingLight === null &&
    data.flags === SceneFlags.NONE && data.cullingFlags === CullingFlags.CAMERA_FRUSTUM, 'SceneData reset');
check(pool.createDispatch() === dispatch && dispatch.material === null && dispatch.passID === 0 &&
    [dispatch.threadGroupCountX, dispatch.threadGroupCountY, dispatch.threadGroupCountZ].every(n => n === 0), 'Dispatch reset');
check(pool.createBlit() === blit && blit.models === models && models.length === 0 && blit.camera === null &&
    blit.material === null && blit.passID === 0 && blit.sceneFlags === SceneFlags.NONE && blit.blitType === BlitType.FULLSCREEN_QUAD, 'Blit reset');
check(pool.createRenderData() === render && render.constants === constants && render.buffers === buffers &&
    render.textures === textures && render.samplers === samplers && render.custom === '' &&
    [constants.size, buffers.size, textures.size, samplers.size].every(n => n === 0), 'RenderData reset');

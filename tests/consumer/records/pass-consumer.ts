import { ShaderStageFlagBit } from '../../gfx';
import { ComputeSubpass, RasterSubpass, RenderGraphObjectPool } from './render-graph';
import { RenderCommonObjectPool } from './types';

function check(ok: boolean, field: string): void {
    if (!ok) throw new Error(field);
}

// Capture each container before recycling so replacing it with an empty one also fails the test.
function checkMapAfterReset<K, V>(read: () => Map<K, V>, key: K, value: V): () => boolean {
    const map = read();
    map.set(key, value);
    return () => read() === map && map.size === 0;
}

function checkArrayAfterReset<T>(read: () => T[], value: T): () => boolean {
    const array = read();
    array.push(value);
    return () => read() === array && array.length === 0;
}

const common = new RenderCommonObjectPool();
const pool = new RenderGraphObjectPool(common);
const defaults = new RasterSubpass();
check(defaults.subpassID === 0xFFFFFFFF && defaults.count === 1 && defaults.quality === 0 &&
    new ComputeSubpass().subpassID === 0xFFFFFFFF, 'subpass constructor defaults');

const raster = pool.createRasterSubpass(3, 8, 2);
const subpass = pool.createComputeSubpass(5);
check(raster.subpassID === 3 && raster.count === 8 && raster.quality === 2 && subpass.subpassID === 5, 'subpass arguments');
const compute = pool.createComputePass();
const resolve = pool.createResolvePass();
const copy = pool.createCopyPass();
const move = pool.createMovePass();
const raytrace = pool.createRaytracePass();

const view = pool.createRasterView();
const inputs = [pool.createComputeView()];
const checks = {
    'RasterSubpass.rasterViews': checkMapAfterReset(() => raster.rasterViews, 'color', view),
    'RasterSubpass.computeViews': checkMapAfterReset(() => raster.computeViews, 'input', inputs),
    'RasterSubpass.resolvePairs': checkArrayAfterReset(() => raster.resolvePairs, common.createResolvePair()),
    'ComputeSubpass.rasterViews': checkMapAfterReset(() => subpass.rasterViews, 'color', view),
    'ComputeSubpass.computeViews': checkMapAfterReset(() => subpass.computeViews, 'input', inputs),
    'ComputePass.computeViews': checkMapAfterReset(() => compute.computeViews, 'input', inputs),
    'ComputePass.textures': checkMapAfterReset(() => compute.textures, 'image', ShaderStageFlagBit.COMPUTE),
    'ResolvePass.resolvePairs': checkArrayAfterReset(() => resolve.resolvePairs, common.createResolvePair()),
    'CopyPass.copyPairs': checkArrayAfterReset(() => copy.copyPairs, common.createCopyPair()),
    'CopyPass.uploadPairs': checkArrayAfterReset(() => copy.uploadPairs, common.createUploadPair()),
    'MovePass.movePairs': checkArrayAfterReset(() => move.movePairs, common.createMovePair()),
    'RaytracePass.computeViews': checkMapAfterReset(() => raytrace.computeViews, 'input', inputs),
};
const viewport = raster.viewport;
viewport.left = viewport.top = viewport.width = viewport.height = 32;
viewport.minDepth = viewport.maxDepth = 0.5;
raster.showStatistics = true;
pool.reset();
check(pool.createRasterSubpass(7, 4, 1) === raster && raster.subpassID === 7 && raster.count === 4 &&
    raster.quality === 1 && !raster.showStatistics, 'RasterSubpass reuse');
check(pool.createComputeSubpass(9) === subpass && subpass.subpassID === 9, 'ComputeSubpass reuse');
check(raster.viewport === viewport && viewport.left === 0 && viewport.top === 0 && viewport.width === 0 &&
    viewport.height === 0 && viewport.minDepth === 0 && viewport.maxDepth === 1, 'RasterSubpass.viewport');
check(pool.createComputePass() === compute && pool.createResolvePass() === resolve && pool.createCopyPass() === copy &&
    pool.createMovePass() === move && pool.createRaytracePass() === raytrace, 'pass reuse');
for (const field of Object.keys(checks) as (keyof typeof checks)[]) check(checks[field](), field);

pool.reset();
check(pool.createRasterSubpass() === raster && raster.subpassID === 0xFFFFFFFF && raster.count === 1 && raster.quality === 0 &&
    pool.createComputeSubpass() === subpass && subpass.subpassID === 0xFFFFFFFF, 'subpass factory defaults');

import { ProgramGroup, ProgramInfo } from './web-types';
import type * as Legacy from './legacy-web-types';
import { PipelineCapabilities, PipelineType, SubpassCapabilities } from './pipeline';
import type { RenderingModule } from './pipeline';

import type { ProgramLibrary, ProgramProxy } from './private';
import type * as LegacyPrivate from './legacy-private';

type Compatible<T extends U, U> = T;
export type EngineLibrary = Compatible<ProgramLibrary, LegacyPrivate.ProgramLibrary>;
export type GeneratedLibrary = Compatible<LegacyPrivate.ProgramLibrary, ProgramLibrary>;
export type EngineProxy = Compatible<ProgramProxy, LegacyPrivate.ProgramProxy>;
export type GeneratedProxy = Compatible<LegacyPrivate.ProgramProxy, ProgramProxy>;

const args: ConstructorParameters<typeof Legacy.ProgramInfo> = [{ name: 'p' }, { name: 's' }, [], [16], { tint: 3 }];
const program: Legacy.ProgramInfo = new ProgramInfo(...args);
const group: Legacy.ProgramGroup = new ProgramGroup();
group.programInfos.set('p', program);
if (group.programInfos.get('p') !== program || program.handleMap !== args[4] || new ProgramGroup().programInfos.size !== 0) {
    throw Error('ProgramInfo arguments or ProgramGroup ownership changed');
}
if (false) {
    // @ts-expect-error All five engine constructor arguments are required.
    new ProgramInfo();
}
const moduleIds: RenderingModule = { getPassID: () => 2, getSubpassID: (pass) => pass + 1, getPhaseID: (pass) => pass + 2 };
const caps = new PipelineCapabilities();
if (caps.subpass !== SubpassCapabilities.NONE || PipelineType.STANDARD !== 1 || moduleIds.getPhaseID(2, 'default') !== 4) {
    throw Error('Pipeline capabilities or module lookup changed');
}

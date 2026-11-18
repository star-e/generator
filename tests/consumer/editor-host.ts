export class Material {}
export class Texture2D {}
export const CCBoolean = Boolean;
export const CCFloat = Number;
export const CCInteger = Number;

export namespace gfx {
    export enum SampleCount { X4 = 4 }
}

export const registeredEnums: object[] = [];
export function ccenum(type: object): void {
    registeredEnums.push(type);
}

const noop = (...args: unknown[]): void => {};
function property(target: object, key: string): void;
function property(options: object): typeof noop;
function property(options: object, key?: string): void | typeof noop {
    if (key === undefined) return noop;
}
export const _decorator = {
    ccclass: (name: string) => noop,
    menu: (name: string) => noop,
    requireComponent: (component: typeof Camera) => noop,
    disallowMultiple: noop,
    executeInEditMode: noop,
    property,
    type: (value: object) => noop,
};

export class Camera {
    readonly camera = { pipelineSettings: null as object | null };
}

export class Component {
    private readonly cameraComponent = new Camera();
    getComponent(type: typeof Camera): Camera {
        return this.cameraComponent;
    }
}

let preview: object | null = null;
export const rendering = {
    setEditorPipelineSettings(settings: object | null): void { preview = settings; },
    getEditorPipelineSettings(): object | null { return preview; },
};

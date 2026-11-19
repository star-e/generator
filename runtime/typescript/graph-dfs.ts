import { GraphColor } from './graph';
import type {
    edge_descriptor, GraphVisitor, IncidenceGraph, MutableVertexPropertyMap,
    TerminatorFunc, vertex_descriptor, VertexListGraph,
} from './graph';

interface Frame {
    vertex: vertex_descriptor;
    edges: Iterator<edge_descriptor> | null;
    incoming?: edge_descriptor;
}

/** Visit one rooted tree using the caller's existing colors. */
export function depthFirstVisit (
    g: IncidenceGraph,
    root: vertex_descriptor,
    visitor: GraphVisitor,
    color: MutableVertexPropertyMap<GraphColor>,
    terminator?: TerminatorFunc,
): void {
    function discover (vertex: vertex_descriptor, incoming?: edge_descriptor): Frame {
        color.put(vertex, GraphColor.GRAY);
        visitor.discoverVertex(vertex, g);
        const edges = terminator && terminator.terminate(vertex, g) ? null : g.oe(vertex);
        return { vertex, edges, incoming };
    }

    visitor.startVertex(root, g);
    const stack = [discover(root)];
    while (stack.length) {
        const frame = stack[stack.length - 1];
        const next = frame.edges && frame.edges.next();
        if (!next || next.done) {
            color.put(frame.vertex, GraphColor.BLACK);
            visitor.finishVertex(frame.vertex, g);
            if (frame.incoming) visitor.finishEdge(frame.incoming, g);
            stack.pop();
            continue;
        }

        const edge = next.value;
        visitor.examineEdge(edge, g);
        const target = g.target(edge);
        const targetColor = color.get(target);
        if (targetColor === GraphColor.WHITE) {
            visitor.treeEdge(edge, g);
            stack.push(discover(target, edge));
        } else {
            if (targetColor === GraphColor.GRAY) visitor.backEdge(edge, g);
            else visitor.forwardOrCrossEdge(edge, g);
            visitor.finishEdge(edge, g);
        }
    }
}

/** Visit the entire forest, starting with the requested root when supplied. */
export function depthFirstSearch (
    g: IncidenceGraph & VertexListGraph,
    visitor: GraphVisitor,
    color: MutableVertexPropertyMap<GraphColor>,
    startVertex: vertex_descriptor | null = null,
): void {
    if (g.nv() === 0) return;
    for (const v of g.v()) {
        color.put(v, GraphColor.WHITE);
        visitor.initializeVertex(v, g);
    }
    if (startVertex !== null) depthFirstVisit(g, startVertex, visitor, color);
    for (const v of g.v()) {
        if (color.get(v) === GraphColor.WHITE) depthFirstVisit(g, v, visitor, color);
    }
}

export function createStore(initial) {
  let state = structuredClone(initial);
  const listeners = new Set();

  return {
    get() {
      return state;
    },
    patch(mutator) {
      const next = mutator(state);
      if (next === state) {
        return state;
      }
      state = next;
      listeners.forEach((fn) => fn(state));
      return state;
    },
    subscribe(fn) {
      listeners.add(fn);
      fn(state);
      return () => listeners.delete(fn);
    }
  };
}

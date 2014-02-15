var global_registers_writable = 0;
describe("Global registers", function() {
  describe('writable', function() {
    it("should reject target", function() {
      var global = Function('return this;')();
      expect(global_registers_writable).toBe(0);
      expect(global.global_registers_writable).toBe(0);
      Object.defineProperty(global, 'global_registers_writable', {
        writable: false
      });
      global_registers_writable = 1;
      expect(global_registers_writable).toBe(0);
    });
  });
});

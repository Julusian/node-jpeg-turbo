const { bufferSize, SAMP_420, SAMP_GRAY, SAMP_444 } = require("..");

describe("buffersize", () => {
  test("check options", () => {
    expect(() => bufferSize()).toThrow('Not enough arguments');
    expect(() => bufferSize(null)).toThrow('Invalid options');
    expect(() => bufferSize(1)).toThrow('Invalid options');
    expect(() => bufferSize("")).toThrow('Invalid options');
    expect(() => bufferSize({})).toThrow('Invalid width');

    expect(() =>
      bufferSize({
        width: {},
        height: {}
      })
    ).toThrow('Invalid width');
    expect(() =>
      bufferSize({
        width: "abc",
        height: "abc"
      })
    ).toThrow('Invalid width');
    expect(() =>
      bufferSize({
        width: null,
        height: null
      })
    ).toThrow('Invalid width');
  });

  test("check result", () => {
    // Checking only approximate ranges
    const size1 = bufferSize({
      width: 10,
      height: 10
    });
    expect(size1).toBeGreaterThan(100);
    expect(size1).toBeLessThan(10000);

    const size2 = bufferSize({
      width: 0,
      height: 0
    });
    expect(size2).toEqual(Math.pow(2, 32) - 1);

    const size3 = bufferSize({
      width: 1000,
      height: 500
    });
    expect(size3).toBeGreaterThan(100000);
    expect(size3).toBeLessThan(4000000);

    const size4 = bufferSize({
      width: 0,
      height: -1
    });
    expect(size4).toEqual(Math.pow(2, 32) - 1);
  });

  test("check options: subsampling", () => {
    expect(() =>
      bufferSize({
        width: 10,
        height: 10,
        subsampling: 9999
      })
    ).toThrow('Invalid subsampling');
    expect(() =>
      bufferSize({
        width: 10,
        height: 10,
        subsampling: -1
      })
    ).toThrow('Invalid subsampling');

    expect(() =>
      bufferSize({
        width: 10,
        height: 10,
        subsampling: "abc"
      })
    ).toThrow('Invalid subsampling');
    expect(() =>
      bufferSize({
        width: 10,
        height: 10,
        subsampling: null
      })
    ).toThrow('Invalid subsampling');
    expect(() =>
      bufferSize({
        width: 10,
        height: 10,
        subsampling: {}
      })
    ).toThrow('Invalid subsampling');
  });

  test("check result: subsampling", () => {
    // Checking only approximate ranges
    const size1 = bufferSize({
      width: 50,
      height: 50
    });

    const size2 = bufferSize({
      width: 50,
      height: 50,
      subsampling: SAMP_420 // should be default
    });
    expect(size2).toEqual(size1);

    const size3 = bufferSize({
      width: 50,
      height: 50,
      subsampling: SAMP_GRAY
    });
    expect(size3).toBeLessThan(size1);
    expect(size3).toBeGreaterThan(5000);

    const size4 = bufferSize({
      width: 50,
      height: 50,
      subsampling: SAMP_444
    });
    expect(size4).toBeGreaterThan(size1);
    expect(size4).toBeLessThan(50000);
  });
});

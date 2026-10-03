#!/usr/bin/env node
// Fallback for when no prebuilt binary is available.
// cmake-js is intentionally not a dependency, as it is rarely needed. If it is not
// already installed, it is fetched on demand using the package manager running the install.

const { spawnSync } = require("child_process");
const path = require("path");

const packageDir = path.join(__dirname, "..");

// Use the same cmake-js version the repo builds with.
// devDependencies are preserved in the published package.json
const CMAKE_JS_RANGE = require(path.join(packageDir, "package.json"))
  .devDependencies["cmake-js"];
const CMAKE_JS_ARGS = ["compile", "--target", "jpeg-turbo"];
const isWindows = process.platform === "win32";

function run(command, args, useShell) {
  console.log(`> ${command} ${args.join(" ")}`);
  return spawnSync(command, args, {
    cwd: packageDir,
    stdio: "inherit",
    shell: useShell && isWindows,
  });
}

function findLocalCmakeJs() {
  try {
    const pkgPath = require.resolve("cmake-js/package.json", {
      paths: [packageDir, process.cwd()],
    });
    const pkg = require(pkgPath);
    const bin = typeof pkg.bin === "string" ? pkg.bin : pkg.bin["cmake-js"];
    return path.join(path.dirname(pkgPath), bin);
  } catch (e) {
    return null;
  }
}

function getRemoteCommand() {
  // eg "pnpm/10.28.0 npm/? node/v22.22.0 linux x64"
  const userAgent = process.env.npm_config_user_agent || "";
  const match = userAgent.match(/^(\w+)\/(\d+)/);
  const pm = match ? match[1] : null;
  const pmMajor = match ? Number(match[2]) : 0;

  if (pm === "pnpm") {
    return ["pnpm", ["dlx", `cmake-js@${CMAKE_JS_RANGE}`, ...CMAKE_JS_ARGS]];
  } else if (pm === "yarn" && pmMajor >= 2) {
    return [
      "yarn",
      ["dlx", "-p", `cmake-js@${CMAKE_JS_RANGE}`, "cmake-js", ...CMAKE_JS_ARGS],
    ];
  } else {
    return [
      "npm",
      [
        "exec",
        "--yes",
        `--package=cmake-js@${CMAKE_JS_RANGE}`,
        "--",
        "cmake-js",
        ...CMAKE_JS_ARGS,
      ],
    ];
  }
}

function printHelp() {
  const libc = process.platform === "linux" && isMusl() ? "-musl" : "";
  const reason = process.env.npm_config_build_from_source
    ? "Building from source was requested, but it failed."
    : `No prebuilt binary is available for ${process.platform}-${process.arch}${libc}, and building from source failed.`;
  console.error(`
@julusian/jpeg-turbo: ${reason}

Building from source requires cmake, a C++ toolchain and nasm (on x86/x64).
It also requires cmake-js, which is normally fetched automatically. If that is not possible in your environment, install 'cmake-js' alongside this package, or clone the repository and build it yourself.
`);
}

function isMusl() {
  try {
    return !process.report.getReport().header.glibcVersionRuntime;
  } catch (e) {
    return false;
  }
}

const localCmakeJs = findLocalCmakeJs();
const result = localCmakeJs
  ? run(process.execPath, [localCmakeJs, ...CMAKE_JS_ARGS], false)
  : run(...getRemoteCommand(), true);

if (result.error || result.status !== 0) {
  if (result.error) console.error(result.error.message);
  printHelp();
  process.exit(result.status || 1);
}

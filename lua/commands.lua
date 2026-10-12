local pyxis = require "pyxis"
local commands = {}

local function operation_error(result, operation, stream, message, code)
  if not result.operation_error then
    result.operation_error = {
      operation = operation, stream = stream, message = message, code = code,
    }
  end
end

local function read_output(job, result, stream)
  local output = result[stream .. "_output"]
  if not output.valid then
    operation_error(result, "read", stream, "capture output is unavailable", pyxis.CALL_UNAVAILABLE)
    return nil, pyxis.CALL_UNAVAILABLE
  end
  local chunks, offset = {}, 0
  while offset < output.returned_size do
    local count = output.returned_size - offset
    if count > pyxis.READ_MAX_BYTES then count = pyxis.READ_MAX_BYTES end
    local bytes, status = job:read(stream, offset, count)
    if status ~= pyxis.CALL_OK then
      operation_error(result, "read", stream, "native capture read failed", status)
      return table.concat(chunks), status
    end
    if #bytes == 0 then
      operation_error(result, "read", stream, "capture ended before its recorded size", pyxis.CALL_IO)
      return table.concat(chunks), pyxis.CALL_IO
    end
    chunks[#chunks + 1], offset = bytes, offset + #bytes
  end
  return table.concat(chunks), pyxis.CALL_OK
end

local function finish(job, status)
  if not job then
    return nil, nil, nil, status, nil
  end
  local result, wait_status = job:wait()
  if status == pyxis.CALL_OK then status = wait_status end
  if not result.complete then
    if status == pyxis.CALL_OK then status = pyxis.CALL_BUSY end
    return nil, nil, result, status, job
  end
  local stdout, stdout_status = read_output(job, result, "stdout")
  local stderr, stderr_status = read_output(job, result, "stderr")
  if status == pyxis.CALL_OK then status = stdout_status end
  if status == pyxis.CALL_OK then status = stderr_status end
  local close_status = job:close()
  if status == pyxis.CALL_OK then status = close_status end
  if close_status ~= pyxis.CALL_BUSY then job = nil end
  return stdout, stderr, result, status, job
end

function commands.capture(argv, options)
  return finish(pyxis.job(argv, options))
end

function commands.pipeline(stages, options)
  return finish(pyxis.pipeline(stages, options))
end

local function replay(file, bytes, result, stream)
  local delivery = { attempted = #bytes, accepted = 0, flushed = false, uncertain = false }
  result.delivery[stream] = delivery
  local called, value, message, code = pcall(file.write, file, bytes)
  if not called or not value then
    delivery.accepted = nil
    delivery.uncertain = true
    operation_error(result, "write", stream, called and message or value, code)
    return pyxis.CALL_IO
  end
  delivery.accepted = #bytes
  called, value, message, code = pcall(file.flush, file)
  if not called or not value then
    delivery.uncertain = true
    operation_error(result, "flush", stream, called and message or value, code)
    return pyxis.CALL_IO
  end
  delivery.flushed = true
  return pyxis.CALL_OK
end

function commands.run(argv, options)
  assert(type(options) == "table", "expected command options")
  local output = options.stdout or io.stdout
  local error_output = options.stderr or io.stderr
  assert(io.type(output) == "file" and io.type(error_output) == "file",
    "expected open Lua output FILEs")
  local stdout, stderr, result, status, job = commands.capture(argv, options)
  if result and result.complete and not result.operation_error then
    result.delivery = {}
    local stdout_status = replay(output, stdout, result, "stdout")
    local stderr_status = replay(error_output, stderr, result, "stderr")
    if status == pyxis.CALL_OK then status = stdout_status end
    if status == pyxis.CALL_OK then status = stderr_status end
  end
  return result, status, job
end

return commands

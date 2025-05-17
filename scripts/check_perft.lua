local posix = require("posix")

function starts_with(haystack, start)
   return string.sub(haystack, 1, string.len(start)) == start
end

function popen2(cmd)
    local r1, w1 = posix.pipe()
    local r2, w2 = posix.pipe()
    assert(r1 ~= nil or r2 ~= nil, "pipe() failed")
    local pid, err = posix.fork()
    assert(pid ~= nil, "fork() failed")
    if pid == 0 then
        posix.close(w1)
        posix.close(r2)
        posix.dup2(r1, posix.fileno(io.stdin))
        posix.dup2(w2, posix.fileno(io.stdout))
        posix.close(r1)
        posix.close(w2)
        local cmd0 = table.remove(cmd, 1)
        local ret, err = posix.execp(cmd0, cmd)
        assert(ret ~= nil, "execp() failed")
        posix._exit(1)
        return
    end
    posix.close(r1)
    posix.close(w2)
    return pid, w1, r2
end

function run(cmd, stdin)
    local pid, sw, sr = popen2(cmd)
    posix.write(sw, stdin)
    local output = ""
    while true do
        output = output .. posix.read(sr, 4096)
        _, status = posix.wait(pid, posix.WNOHANG)
        if status ~= "running" then break end
    end
    output = output .. posix.read(sr, 4096)
    posix.close(sr)
    posix.close(sw)
    return output
end

function runPerft(cmd, perft_cmd, fen, moves, depth)
    if moves ~= nil and moves ~= "" then moves = " moves " .. moves else moves = "" end
    local output = run(cmd, "setoption name UCI_Chess960 value true\nposition fen " .. fen .. moves .. "\n" .. perft_cmd .. " " .. depth .. "\nquit\n")
    local result = {}
    for line in output:gmatch("[^\r\n]+") do
        local colon = string.find(line, ":")
        if colon ~= nil and not starts_with(line, "info") then
            local move = line:sub(1, colon - 1)
            local count = line:sub(colon + 2)
            if starts_with(move, "Nodes searched") then
                result["total"] = count
            else
                result[move] = count
            end
        end
    end
    return result
end

function comparePerft(fen, move_str, depth)
    local s = runPerft({"stockfish"}, "go perft", fen, move_str, depth)
    local b = runPerft({"./rose"}, "perft", fen, move_str, depth)

    local moves = {}
    for k, _ in pairs(s) do
        moves[k] = true
    end
    for k, _ in pairs(b) do
        moves[k] = true
    end
    if move_str == nil then
        print("depth " .. depth .. " for position fen " .. fen)
    else
        print("drill-into depth " .. depth .. " for position fen " .. fen .. " moves" .. move_str)
    end
    local mismatch = {}
    for k, _ in pairs(moves) do
        if s[k] ~= b[k] and k ~= "total" then
            table.insert(mismatch, k)
            local sk = s[k]
            local bk = b[k]
            if sk == nil then sk = "0" end
            if bk == nil then bk = "0" end
            print(k .. " stockfish " .. sk .. " rose " .. bk)
        end
    end
    print("total" .. " stockfish " .. s["total"] .. " rose " .. b["total"])
    return mismatch
end

function drillIntoPerft(fen, max_depth)
    for depth = 1, max_depth do
        local miss = comparePerft(fen, nil, depth)
        if #miss > 0 then
            local move = ""
            for miss_depth = (depth - 1), 1, -1 do
                print("---")
                move = move .. " " .. miss[1]
                miss = comparePerft(fen, move, miss_depth)
            end
            print("===")
            posix._exit(1)
        end
    end
end

drillIntoPerft("3k1b2/p4r2/1P6/P1p1pnpP/2P5/R4B1R/6N1/4K3 b - - 0 1", 6)
drillIntoPerft("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", 6)
drillIntoPerft("r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1", 5)
drillIntoPerft("8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1", 6)
drillIntoPerft("r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1", 6)
drillIntoPerft("rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8", 5)
drillIntoPerft("r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10", 5)
drillIntoPerft("4k3/8/8/8/8/8/8/4K2R w K - 0 1", 6)
drillIntoPerft("4k3/8/8/8/8/8/8/R3K3 w Q - 0 1", 6)
drillIntoPerft("4k2r/8/8/8/8/8/8/4K3 w k - 0 1", 6)
drillIntoPerft("r3k3/8/8/8/8/8/8/4K3 w q - 0 1", 6)
drillIntoPerft("4k3/8/8/8/8/8/8/R3K2R w KQ - 0 1", 6)
drillIntoPerft("r3k2r/8/8/8/8/8/8/4K3 w kq - 0 1", 6)
drillIntoPerft("8/8/8/8/8/8/6k1/4K2R w K - 0 1", 6)
drillIntoPerft("8/8/8/8/8/8/1k6/R3K3 w Q - 0 1", 6)
drillIntoPerft("4k2r/6K1/8/8/8/8/8/8 w k - 0 1", 6)
drillIntoPerft("r3k3/1K6/8/8/8/8/8/8 w q - 0 1", 6)
drillIntoPerft("r3k2r/8/8/8/8/8/8/R3K2R w KQkq - 0 1", 6)
drillIntoPerft("r3k2r/8/8/8/8/8/8/1R2K2R w Kkq - 0 1", 6)
drillIntoPerft("r3k2r/8/8/8/8/8/8/2R1K2R w Kkq - 0 1", 6)
drillIntoPerft("r3k2r/8/8/8/8/8/8/R3K1R1 w Qkq - 0 1", 6)
drillIntoPerft("1r2k2r/8/8/8/8/8/8/R3K2R w KQk - 0 1", 6)
drillIntoPerft("2r1k2r/8/8/8/8/8/8/R3K2R w KQk - 0 1", 6)
drillIntoPerft("r3k1r1/8/8/8/8/8/8/R3K2R w KQq - 0 1", 6)
drillIntoPerft("4k3/8/8/8/8/8/8/4K2R b K - 0 1", 6)
drillIntoPerft("4k3/8/8/8/8/8/8/R3K3 b Q - 0 1", 6)
drillIntoPerft("4k2r/8/8/8/8/8/8/4K3 b k - 0 1", 6)
drillIntoPerft("r3k3/8/8/8/8/8/8/4K3 b q - 0 1", 6)
drillIntoPerft("4k3/8/8/8/8/8/8/R3K2R b KQ - 0 1", 6)
drillIntoPerft("r3k2r/8/8/8/8/8/8/4K3 b kq - 0 1", 6)
drillIntoPerft("8/8/8/8/8/8/6k1/4K2R b K - 0 1", 6)
drillIntoPerft("8/8/8/8/8/8/1k6/R3K3 b Q - 0 1", 6)
drillIntoPerft("4k2r/6K1/8/8/8/8/8/8 b k - 0 1", 6)
drillIntoPerft("r3k3/1K6/8/8/8/8/8/8 b q - 0 1", 6)
drillIntoPerft("r3k2r/8/8/8/8/8/8/R3K2R b KQkq - 0 1", 6)
drillIntoPerft("r3k2r/8/8/8/8/8/8/1R2K2R b Kkq - 0 1", 6)
drillIntoPerft("r3k2r/8/8/8/8/8/8/2R1K2R b Kkq - 0 1", 6)
drillIntoPerft("r3k2r/8/8/8/8/8/8/R3K1R1 b Qkq - 0 1", 6)
drillIntoPerft("1r2k2r/8/8/8/8/8/8/R3K2R b KQk - 0 1", 6)
drillIntoPerft("2r1k2r/8/8/8/8/8/8/R3K2R b KQk - 0 1", 6)
drillIntoPerft("r3k1r1/8/8/8/8/8/8/R3K2R b KQq - 0 1", 6)
drillIntoPerft("8/1n4N1/2k5/8/8/5K2/1N4n1/8 w - - 0 1", 6)
drillIntoPerft("8/1k6/8/5N2/8/4n3/8/2K5 w - - 0 1", 6)
drillIntoPerft("8/8/4k3/3Nn3/3nN3/4K3/8/8 w - - 0 1", 6)
drillIntoPerft("K7/8/2n5/1n6/8/8/8/k6N w - - 0 1", 6)
drillIntoPerft("k7/8/2N5/1N6/8/8/8/K6n w - - 0 1", 6)
drillIntoPerft("8/1n4N1/2k5/8/8/5K2/1N4n1/8 b - - 0 1", 6)
drillIntoPerft("8/1k6/8/5N2/8/4n3/8/2K5 b - - 0 1", 6)
drillIntoPerft("8/8/3K4/3Nn3/3nN3/4k3/8/8 b - - 0 1", 6)
drillIntoPerft("K7/8/2n5/1n6/8/8/8/k6N b - - 0 1", 6)
drillIntoPerft("k7/8/2N5/1N6/8/8/8/K6n b - - 0 1", 6)
drillIntoPerft("B6b/8/8/8/2K5/4k3/8/b6B w - - 0 1", 6)
drillIntoPerft("8/8/1B6/7b/7k/8/2B1b3/7K w - - 0 1", 6)
drillIntoPerft("k7/B7/1B6/1B6/8/8/8/K6b w - - 0 1", 6)
drillIntoPerft("K7/b7/1b6/1b6/8/8/8/k6B w - - 0 1", 6)
drillIntoPerft("B6b/8/8/8/2K5/5k2/8/b6B b - - 0 1", 6)
drillIntoPerft("8/8/1B6/7b/7k/8/2B1b3/7K b - - 0 1", 6)
drillIntoPerft("k7/B7/1B6/1B6/8/8/8/K6b b - - 0 1", 6)
drillIntoPerft("K7/b7/1b6/1b6/8/8/8/k6B b - - 0 1", 6)
drillIntoPerft("7k/RR6/8/8/8/8/rr6/7K w - - 0 1", 6)
drillIntoPerft("R6r/8/8/2K5/5k2/8/8/r6R w - - 0 1", 6)
drillIntoPerft("7k/RR6/8/8/8/8/rr6/7K b - - 0 1", 6)
drillIntoPerft("R6r/8/8/2K5/5k2/8/8/r6R b - - 0 1", 6)
drillIntoPerft("6kq/8/8/8/8/8/8/7K w - - 0 1", 6)
drillIntoPerft("6KQ/8/8/8/8/8/8/7k b - - 0 1", 6)
drillIntoPerft("K7/8/8/3Q4/4q3/8/8/7k w - - 0 1", 6)
drillIntoPerft("6qk/8/8/8/8/8/8/7K b - - 0 1", 6)
drillIntoPerft("6KQ/8/8/8/8/8/8/7k b - - 0 1", 6)
drillIntoPerft("K7/8/8/3Q4/4q3/8/8/7k b - - 0 1", 6)
drillIntoPerft("8/8/8/8/8/K7/P7/k7 w - - 0 1", 6)
drillIntoPerft("8/8/8/8/8/7K/7P/7k w - - 0 1", 6)
drillIntoPerft("K7/p7/k7/8/8/8/8/8 w - - 0 1", 6)
drillIntoPerft("7K/7p/7k/8/8/8/8/8 w - - 0 1", 6)
drillIntoPerft("8/2k1p3/3pP3/3P2K1/8/8/8/8 w - - 0 1", 6)
drillIntoPerft("8/8/8/8/8/K7/P7/k7 b - - 0 1", 6)
drillIntoPerft("8/8/8/8/8/7K/7P/7k b - - 0 1", 6)
drillIntoPerft("K7/p7/k7/8/8/8/8/8 b - - 0 1", 6)
drillIntoPerft("7K/7p/7k/8/8/8/8/8 b - - 0 1", 6)
drillIntoPerft("8/2k1p3/3pP3/3P2K1/8/8/8/8 b - - 0 1", 6)
drillIntoPerft("8/8/8/8/8/4k3/4P3/4K3 w - - 0 1", 6)
drillIntoPerft("4k3/4p3/4K3/8/8/8/8/8 b - - 0 1", 6)
drillIntoPerft("8/8/7k/7p/7P/7K/8/8 w - - 0 1", 6)
drillIntoPerft("8/8/k7/p7/P7/K7/8/8 w - - 0 1", 6)
drillIntoPerft("8/8/3k4/3p4/3P4/3K4/8/8 w - - 0 1", 6)
drillIntoPerft("8/3k4/3p4/8/3P4/3K4/8/8 w - - 0 1", 6)
drillIntoPerft("8/8/3k4/3p4/8/3P4/3K4/8 w - - 0 1", 6)
drillIntoPerft("k7/8/3p4/8/3P4/8/8/7K w - - 0 1", 6)
drillIntoPerft("8/8/7k/7p/7P/7K/8/8 b - - 0 1", 6)
drillIntoPerft("8/8/k7/p7/P7/K7/8/8 b - - 0 1", 6)
drillIntoPerft("8/8/3k4/3p4/3P4/3K4/8/8 b - - 0 1", 6)
drillIntoPerft("8/3k4/3p4/8/3P4/3K4/8/8 b - - 0 1", 6)
drillIntoPerft("8/8/3k4/3p4/8/3P4/3K4/8 b - - 0 1", 6)
drillIntoPerft("k7/8/3p4/8/3P4/8/8/7K b - - 0 1", 6)
drillIntoPerft("7k/3p4/8/8/3P4/8/8/K7 w - - 0 1", 6)
drillIntoPerft("7k/8/8/3p4/8/8/3P4/K7 w - - 0 1", 6)
drillIntoPerft("k7/8/8/7p/6P1/8/8/K7 w - - 0 1", 6)
drillIntoPerft("k7/8/7p/8/8/6P1/8/K7 w - - 0 1", 6)
drillIntoPerft("k7/8/8/6p1/7P/8/8/K7 w - - 0 1", 6)
drillIntoPerft("k7/8/6p1/8/8/7P/8/K7 w - - 0 1", 6)
drillIntoPerft("k7/8/8/3p4/4p3/8/8/7K w - - 0 1", 6)
drillIntoPerft("k7/8/3p4/8/8/4P3/8/7K w - - 0 1", 6)
drillIntoPerft("7k/3p4/8/8/3P4/8/8/K7 b - - 0 1", 6)
drillIntoPerft("7k/8/8/3p4/8/8/3P4/K7 b - - 0 1", 6)
drillIntoPerft("k7/8/8/7p/6P1/8/8/K7 b - - 0 1", 6)
drillIntoPerft("k7/8/7p/8/8/6P1/8/K7 b - - 0 1", 6)
drillIntoPerft("k7/8/8/6p1/7P/8/8/K7 b - - 0 1", 6)
drillIntoPerft("k7/8/6p1/8/8/7P/8/K7 b - - 0 1", 6)
drillIntoPerft("k7/8/8/3p4/4p3/8/8/7K b - - 0 1", 6)
drillIntoPerft("k7/8/3p4/8/8/4P3/8/7K b - - 0 1", 6)
drillIntoPerft("7k/8/8/p7/1P6/8/8/7K w - - 0 1", 6)
drillIntoPerft("7k/8/p7/8/8/1P6/8/7K w - - 0 1", 6)
drillIntoPerft("7k/8/8/1p6/P7/8/8/7K w - - 0 1", 6)
drillIntoPerft("7k/8/1p6/8/8/P7/8/7K w - - 0 1", 6)
drillIntoPerft("k7/7p/8/8/8/8/6P1/K7 w - - 0 1", 6)
drillIntoPerft("k7/6p1/8/8/8/8/7P/K7 w - - 0 1", 6)
drillIntoPerft("3k4/3pp3/8/8/8/8/3PP3/3K4 w - - 0 1", 6)
drillIntoPerft("7k/8/8/p7/1P6/8/8/7K b - - 0 1", 6)
drillIntoPerft("7k/8/p7/8/8/1P6/8/7K b - - 0 1", 6)
drillIntoPerft("7k/8/8/1p6/P7/8/8/7K b - - 0 1", 6)
drillIntoPerft("7k/8/1p6/8/8/P7/8/7K b - - 0 1", 6)
drillIntoPerft("k7/7p/8/8/8/8/6P1/K7 b - - 0 1", 6)
drillIntoPerft("k7/6p1/8/8/8/8/7P/K7 b - - 0 1", 6)
drillIntoPerft("3k4/3pp3/8/8/8/8/3PP3/3K4 b - - 0 1", 6)
drillIntoPerft("8/Pk6/8/8/8/8/6Kp/8 w - - 0 1", 6)
drillIntoPerft("n1n5/1Pk5/8/8/8/8/5Kp1/5N1N w - - 0 1", 6)
drillIntoPerft("8/PPPk4/8/8/8/8/4Kppp/8 w - - 0 1", 6)
drillIntoPerft("n1n5/PPPk4/8/8/8/8/4Kppp/5N1N w - - 0 1", 6)
drillIntoPerft("8/Pk6/8/8/8/8/6Kp/8 b - - 0 1", 6)
drillIntoPerft("n1n5/1Pk5/8/8/8/8/5Kp1/5N1N b - - 0 1", 6)
drillIntoPerft("8/PPPk4/8/8/8/8/4Kppp/8 b - - 0 1", 6)
drillIntoPerft("n1n5/PPPk4/8/8/8/8/4Kppp/5N1N b - - 0 1", 6)
-- dfrc
drillIntoPerft("2r1kr2/8/8/8/8/8/8/1R2K1R1 w GBfc - 0 1", 6)
drillIntoPerft("rkr5/8/8/8/8/8/8/5RKR w HFca - 0 1", 6)
drillIntoPerft("2r3kr/8/8/8/8/8/8/2KRR3 w h - 3 2", 6)
drillIntoPerft("5rkr/8/8/8/8/8/8/RKR5 w CAhf - 0 1", 6)
drillIntoPerft("3rkr2/8/8/8/8/8/8/R3K2R w HAfd - 0 1", 6)
drillIntoPerft("4k3/8/8/8/8/8/8/4KR2 w F - 0 1", 6)
drillIntoPerft("4kr2/8/8/8/8/8/8/4K3 w f - 0 1", 6)
drillIntoPerft("4k3/8/8/8/8/8/8/2R1K3 w C - 0 1", 6)
drillIntoPerft("2r1k3/8/8/8/8/8/8/4K3 w c - 0 1", 6)
print("ok.")

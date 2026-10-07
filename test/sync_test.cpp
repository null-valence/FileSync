#include <gtest/gtest.h>
#include "sync.h"
#include "hasher.h"
#include <chrono>
#include <fstream>

// Test 1
TEST(PlanSync, EmptyDirectories) {
    Snapshot snapshotA;
    Snapshot snapshotB;
    SyncState state;

    auto result = plan_sync(
        snapshotA,
        snapshotB,
        state,
        "/tmp/filesync-test-A",
        "/tmp/filesync-test-B"
    );

    ASSERT_TRUE(result.has_value());

    EXPECT_TRUE(result->actions.empty());
    EXPECT_TRUE(result->conflicts.empty());
}

// Test 2
TEST(PlanSync, AChangedBUnchanged) {
    const fs::path rootA = "/tmp/filesync-test-A";
    const fs::path rootB = "/tmp/filesync-test-B";

    fs::create_directories(rootA);
    fs::create_directories(rootB);

    {
        std::ofstream(rootA / "file.txt") << "NEW";
        std::ofstream(rootB / "file.txt") << "OLD";
    }

    Snapshot snapshotA;
    Snapshot snapshotB;
    SyncState state;

    auto old_time = fs::last_write_time(rootB / "file.txt");
    fs::last_write_time(
        rootA / "file.txt",
        old_time + std::chrono::seconds(1)
    );

    snapshotA.files["file.txt"] = {
        "file.txt",
        3,
        fs::last_write_time(rootA / "file.txt")
    };

    snapshotB.files["file.txt"] = {
        "file.txt",
        3,
        old_time
    };

    state.files["file.txt"] = {
        3,
        old_time,
        *hash_file(rootB / "file.txt")
    };

    auto result = plan_sync(
        snapshotA,
        snapshotB,
        state,
        rootA,
        rootB
    );

    ASSERT_TRUE(result.has_value());

    ASSERT_EQ(result->actions.size(), 1);
    EXPECT_TRUE(result->conflicts.empty());

    EXPECT_EQ(result->actions[0].type, ActionType::Update);
    EXPECT_EQ(result->actions[0].direction, SyncDirection::AtoB);
    EXPECT_EQ(result->actions[0].relative_path, "file.txt");
}

// Test 3
TEST(PlanSync, AUnchangedBChanged) {
    const fs::path rootA = "/tmp/filesync-test-A";
    const fs::path rootB = "/tmp/filesync-test-B";

    fs::create_directories(rootA);
    fs::create_directories(rootB);

    {
        std::ofstream(rootB / "file.txt") << "NEW";
        std::ofstream(rootA / "file.txt") << "OLD";
    }

    Snapshot snapshotA;
    Snapshot snapshotB;
    SyncState state;

    auto old_time = fs::last_write_time(rootA / "file.txt");
    fs::last_write_time(
        rootB / "file.txt",
        old_time + std::chrono::seconds(1)
    );

    snapshotB.files["file.txt"] = {
        "file.txt",
        3,
        fs::last_write_time(rootB / "file.txt")
    };

    snapshotA.files["file.txt"] = {
        "file.txt",
        3,
        old_time
    };

    state.files["file.txt"] = {
        3,
        old_time,
        *hash_file(rootA / "file.txt")
    };

    auto result = plan_sync(
        snapshotA,
        snapshotB,
        state,
        rootA,
        rootB
    );

    ASSERT_TRUE(result.has_value());

    ASSERT_EQ(result->actions.size(), 1);
    EXPECT_TRUE(result->conflicts.empty());

    EXPECT_EQ(result->actions[0].type, ActionType::Update);
    EXPECT_EQ(result->actions[0].direction, SyncDirection::BtoA);
    EXPECT_EQ(result->actions[0].relative_path, "file.txt");
}

// Test 4
TEST(PlanSync, AChangedBChanged_Same) {
    const fs::path rootA = "/tmp/filesync-test-A";
    const fs::path rootB = "/tmp/filesync-test-B";
    const fs::path rootC = "/tmp/filesync-test-C";

    fs::create_directories(rootA);
    fs::create_directories(rootB);
    fs::create_directories(rootC);

    {
        std::ofstream(rootA / "file.txt") << "NEW";
        std::ofstream(rootB / "file.txt") << "NEW";
        std::ofstream(rootC / "file.txt") << "OLD";
    }

    Snapshot snapshotA;
    Snapshot snapshotB;
    SyncState state;

    auto old_time = fs::last_write_time(rootC / "file.txt");

    state.files["file.txt"] = {
        3,
        old_time,
        *hash_file(rootC / "file.txt")
    };

    snapshotA.files["file.txt"] = {
        "file.txt",
        3,
        old_time + std::chrono::seconds(1)
    };

    snapshotB.files["file.txt"] = {
        "file.txt",
        3,
        old_time + std::chrono::seconds(2)
    };

    auto result = plan_sync(
        snapshotA,
        snapshotB,
        state,
        rootA,
        rootB
    );

    ASSERT_TRUE(result.has_value());

    EXPECT_TRUE(result->actions.empty());
    EXPECT_TRUE(result->conflicts.empty());
}

// Test 5
TEST(PlanSync, AChangedBChanged_Diff) {
    const fs::path rootA = "/tmp/filesync-test-A";
    const fs::path rootB = "/tmp/filesync-test-B";
    const fs::path rootC = "/tmp/filesync-test-C";

    fs::create_directories(rootA);
    fs::create_directories(rootB);
    fs::create_directories(rootC);

    {
        std::ofstream(rootA / "file.txt") << "NEW1";
        std::ofstream(rootB / "file.txt") << "NEW2";
        std::ofstream(rootC / "file.txt") << "OLD";
    }

    Snapshot snapshotA;
    Snapshot snapshotB;
    SyncState state;

    auto old_time = fs::last_write_time(rootC / "file.txt");

    state.files["file.txt"] = {
        3,
        old_time,
        *hash_file(rootC / "file.txt")
    };

    snapshotA.files["file.txt"] = {
        "file.txt",
        3,
        old_time + std::chrono::seconds(1)
    };

    snapshotB.files["file.txt"] = {
        "file.txt",
        3,
        old_time + std::chrono::seconds(2)
    };

    auto result = plan_sync(
        snapshotA,
        snapshotB,
        state,
        rootA,
        rootB
    );

    ASSERT_TRUE(result.has_value());

    EXPECT_TRUE(result->actions.empty());
    ASSERT_EQ(result->conflicts.size(), 1);
    EXPECT_EQ(result->conflicts[0].relative_path, "file.txt");
}

// Test 6
TEST(PlanSync, ADeletedBUnchanged) {
    const fs::path rootA = "/tmp/filesync-test-A";
    const fs::path rootB = "/tmp/filesync-test-B";

    fs::create_directories(rootA);
    fs::create_directories(rootB);

    {
        std::ofstream(rootB / "file.txt") << "OLD";
    }

    Snapshot snapshotA;
    Snapshot snapshotB;
    SyncState state;

    auto old_time = fs::last_write_time(rootB / "file.txt");
    fs::last_write_time(
        rootA / "file.txt",
        old_time + std::chrono::seconds(1)
    );

    snapshotB.files["file.txt"] = {
        "file.txt",
        3,
        old_time
    };

    state.files["file.txt"] = {
        3,
        old_time,
        *hash_file(rootB / "file.txt")
    };

    auto result = plan_sync(
        snapshotA,
        snapshotB,
        state,
        rootA,
        rootB
    );

    ASSERT_TRUE(result.has_value());

    ASSERT_EQ(result->actions.size(), 1);
    EXPECT_TRUE(result->conflicts.empty());

    EXPECT_EQ(result->actions[0].type, ActionType::Delete);
    EXPECT_EQ(result->actions[0].direction, SyncDirection::AtoB);
    EXPECT_EQ(result->actions[0].relative_path, "file.txt");
}

// Test 7
TEST(PlanSync, ADeletedBChanged) {
    const fs::path rootA = "/tmp/filesync-test-A";
    const fs::path rootB = "/tmp/filesync-test-B";
    const fs::path rootC = "/tmp/filesync-test-C";

    fs::create_directories(rootA);
    fs::create_directories(rootB);
    fs::create_directories(rootC);

    {
        std::ofstream(rootB / "file.txt") << "NEW";
        std::ofstream(rootC / "file.txt") << "OLD";
    }

    Snapshot snapshotA;
    Snapshot snapshotB;
    SyncState state;

    auto old_time = fs::last_write_time(rootC / "file.txt");

    snapshotB.files["file.txt"] = {
        "file.txt",
        3,
        old_time + std::chrono::seconds(1)
    };

    state.files["file.txt"] = {
        3,
        old_time,
        *hash_file(rootC / "file.txt")
    };

    auto result = plan_sync(
        snapshotA,
        snapshotB,
        state,
        rootA,
        rootB
    );

    ASSERT_TRUE(result.has_value());

    EXPECT_TRUE(result->actions.empty());
    ASSERT_EQ(result->conflicts.size(), 1);
    EXPECT_EQ(result->conflicts[0].relative_path, "file.txt");
}

// Test 8
TEST(PlanSync, AChangedBNoFile) {
    const fs::path rootA = "/tmp/filesync-test-A";
    const fs::path rootB = "/tmp/filesync-test-B";

    fs::create_directories(rootA);
    fs::create_directories(rootB);

    {
        std::ofstream(rootA / "file.txt") << "NEW";
    }

    Snapshot snapshotA;
    Snapshot snapshotB;
    SyncState state;

    snapshotA.files["file.txt"] = {
        "file.txt",
        3,
        fs::last_write_time(rootA / "file.txt")
    };

    auto result = plan_sync(
        snapshotA,
        snapshotB,
        state,
        rootA,
        rootB
    );

    ASSERT_TRUE(result.has_value());

    ASSERT_EQ(result->actions.size(), 1);
    EXPECT_TRUE(result->conflicts.empty());

    EXPECT_EQ(result->actions[0].type, ActionType::Copy);
    EXPECT_EQ(result->actions[0].direction, SyncDirection::AtoB);
    EXPECT_EQ(result->actions[0].relative_path, "file.txt");
}

// Test 9
TEST(PlanSync, ANoFileBChanged) {
    const fs::path rootA = "/tmp/filesync-test-A";
    const fs::path rootB = "/tmp/filesync-test-B";

    fs::create_directories(rootA);
    fs::create_directories(rootB);

    {
        std::ofstream(rootB / "file.txt") << "NEW";
    }

    Snapshot snapshotA;
    Snapshot snapshotB;
    SyncState state;

    snapshotB.files["file.txt"] = {
        "file.txt",
        3,
        fs::last_write_time(rootB / "file.txt")
    };

    auto result = plan_sync(
        snapshotA,
        snapshotB,
        state,
        rootA,
        rootB
    );

    ASSERT_TRUE(result.has_value());

    ASSERT_EQ(result->actions.size(), 1);
    EXPECT_TRUE(result->conflicts.empty());

    EXPECT_EQ(result->actions[0].type, ActionType::Copy);
    EXPECT_EQ(result->actions[0].direction, SyncDirection::BtoA);
    EXPECT_EQ(result->actions[0].relative_path, "file.txt");
}

// Test 10
TEST(PlanSync, AChangedBChanged) {
    const fs::path rootA = "/tmp/filesync-test-A";
    const fs::path rootB = "/tmp/filesync-test-B";

    fs::create_directories(rootA);
    fs::create_directories(rootB);

    {
        std::ofstream(rootB / "file.txt") << "NEW1";
        std::ofstream(rootB / "file.txt") << "NEW2";
    }

    Snapshot snapshotA;
    Snapshot snapshotB;
    SyncState state;

    snapshotA.files["file.txt"] = {
        "file.txt",
        4,
        fs::last_write_time(rootA / "file.txt")
    };
    snapshotB.files["file.txt"] = {
        "file.txt",
        4,
        fs::last_write_time(rootB / "file.txt")
    };

    auto result = plan_sync(
        snapshotA,
        snapshotB,
        state,
        rootA,
        rootB
    );

    ASSERT_TRUE(result.has_value());

    EXPECT_TRUE(result->actions.empty());
    ASSERT_EQ(result->conflicts.size(), 1);
    EXPECT_EQ(result->conflicts[0].relative_path, "file.txt");
}

// Test 11
TEST(State, SaveAndLoad) {
    const fs::path root = "/tmp/filesync-test";

    fs::remove_all(root);
    fs::create_directories(root);

    {
        std::ofstream(root / "file1.txt") << "NEW1";
        std::ofstream(root / "file2.txt") << "NEW_2";
    }

    SyncState state;

    state.files["file1.txt"] = {
        4,
        fs::last_write_time(root / "file1.txt"),
        *hash_file(root / "file1.txt")
    };
    state.files["file2.txt"] = {
        5,
        fs::last_write_time(root / "file2.txt"),
        *hash_file(root / "file2.txt")
    };

    auto result1 = save_state(
        root / "state.txt",
        state
    );

    ASSERT_TRUE(result1.has_value());

    auto result2 = load_state(
        root / "state.txt"
    );
    
    ASSERT_TRUE(result2.has_value());

    const auto& loaded_state = *result2;
    ASSERT_EQ(state.files.size(), loaded_state.files.size());

    for (const auto& [path, file] : state.files) {
        auto it = loaded_state.files.find(path);

        ASSERT_NE(it, loaded_state.files.end());

        EXPECT_EQ(it->second.size, file.size);
        EXPECT_EQ(it->second.last_modified, file.last_modified);
        EXPECT_EQ(it->second.hash, file.hash);
    }
}

// Test 12
TEST(State, SaveAndLoad_Corrupt) {
    const fs::path root = "/tmp/filesync-test";

    fs::remove_all(root);
    fs::create_directories(root);

    {
        std::ofstream(root / "state.txt") << "1\nfile.txt\nnot-a-number\n123456\nabcdef";
    }
    
    auto result = load_state(root / "state.txt");
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), "Invalid state file data");
}

// Test 13
TEST(State, SaveAndLoad_Malform) {
    const fs::path root = "/tmp/filesync-test";

    fs::remove_all(root);
    fs::create_directories(root);

    {
        std::ofstream(root / "state.txt") << "1\nfile.txt\n4";
    }
    
    auto result = load_state(root / "state.txt");
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), "Corrupted state file");
}

// Test 14
TEST(Execute, Copy) {
    const fs::path rootA = "/tmp/filesync-test-A";
    const fs::path rootB = "/tmp/filesync-test-B";

    fs::remove_all(rootA);
    fs::remove_all(rootB);

    fs::create_directories(rootA);
    fs::create_directories(rootB);

    {
        std::ofstream(rootA / "file.txt") << "NEW";
    }
    std::vector<SyncAction> actions;
    actions.push_back({
        ActionType::Copy,
        SyncDirection::AtoB,
        "file.txt"
    });
    
    auto result = execute_sync(actions, rootA, rootB);

    ASSERT_TRUE(result.has_value());
    ASSERT_TRUE(fs::exists(rootB / "file.txt"));

    std::ifstream in(rootB / "file.txt");
    std::string content;
    std::getline(in, content);

    ASSERT_EQ(content, "NEW");
}

// Test 15
TEST(Execute, Copy_Fail) {
    const fs::path rootA = "/tmp/filesync-test-A";
    const fs::path rootB = "/tmp/filesync-test-B";

    fs::remove_all(rootA);
    fs::remove_all(rootB);

    fs::create_directories(rootA);
    fs::create_directories(rootB);

    std::vector<SyncAction> actions;
    actions.push_back({
        ActionType::Copy,
        SyncDirection::AtoB,
        "file.txt"
    });
    
    auto result = execute_sync(actions, rootA, rootB);

    ASSERT_FALSE(result.has_value());
    ASSERT_EQ(result.error(), SyncError::SourceNotFound);
}

// Test 16
TEST(Execute, Delete) {
    const fs::path rootA = "/tmp/filesync-test-A";
    const fs::path rootB = "/tmp/filesync-test-B";

    fs::remove_all(rootA);
    fs::remove_all(rootB);

    fs::create_directories(rootA);
    fs::create_directories(rootB);

    {
        std::ofstream(rootB / "file.txt") << "NEW";
    }
    std::vector<SyncAction> actions;
    actions.push_back({
        ActionType::Delete,
        SyncDirection::AtoB,
        "file.txt"
    });
    
    auto result = execute_sync(actions, rootA, rootB);

    ASSERT_TRUE(result.has_value());
    ASSERT_FALSE(fs::exists(rootB / "file.txt"));
}

// Test 17
TEST(Execute, Update) {
    const fs::path rootA = "/tmp/filesync-test-A";
    const fs::path rootB = "/tmp/filesync-test-B";

    fs::remove_all(rootA);
    fs::remove_all(rootB);

    fs::create_directories(rootA);
    fs::create_directories(rootB);

    {
        std::ofstream(rootA / "file.txt") << "NEW";
        std::ofstream(rootB / "file.txt") << "OLD";
    }
    std::vector<SyncAction> actions;
    actions.push_back({
        ActionType::Update,
        SyncDirection::AtoB,
        "file.txt"
    });
    
    auto result = execute_sync(actions, rootA, rootB);

    ASSERT_TRUE(result.has_value());
    ASSERT_TRUE(fs::exists(rootB / "file.txt"));

    std::ifstream in(rootB / "file.txt");
    std::string content;
    std::getline(in, content);

    ASSERT_EQ(content, "NEW");
}

// Test 18
TEST(State, BuildState) {
    const fs::path root = "/tmp/filesync-test";

    fs::remove_all(root);
    fs::create_directories(root);

    {
        std::ofstream(root / "file.txt") << "NEW";
    }

    auto snapshot = scan_directory(root);
    std::unordered_set<std::string> skipped_conflicts;
    SyncState previous_state;
    auto result = build_state(skipped_conflicts, previous_state, snapshot, root);

    ASSERT_TRUE(result.has_value());

    const auto& state = *result;

    ASSERT_EQ(state.files.size(), 1);

    const auto& file = state.files.at("file.txt");

    ASSERT_EQ(file.size, 3);
    ASSERT_FALSE(file.hash.empty());
}

// Test 19
TEST(Execute, Copy_BtoA) {
    const fs::path rootA = "/tmp/filesync-test-A";
    const fs::path rootB = "/tmp/filesync-test-B";

    fs::remove_all(rootA);
    fs::remove_all(rootB);

    fs::create_directories(rootA);
    fs::create_directories(rootB);

    {
        std::ofstream(rootB / "file.txt") << "FROM B";
    }

    std::vector<SyncAction> actions;
    actions.push_back({
        ActionType::Copy,
        SyncDirection::BtoA,
        "file.txt"
    });

    auto result = execute_sync(actions, rootA, rootB);

    ASSERT_TRUE(result.has_value());
    ASSERT_TRUE(fs::exists(rootA / "file.txt"));

    std::ifstream in(rootA / "file.txt");
    std::string content;
    std::getline(in, content);

    ASSERT_EQ(content, "FROM B");
}

// Test 20
TEST(Conflict, Skip) {
    const fs::path rootA = "/tmp/filesync-test-A";

    fs::remove_all(rootA);

    fs::create_directories(rootA);

    {
        std::ofstream(rootA / "file.txt") << "OLD";
    }

    SyncState previous_state;
    std::unordered_set<std::string> skipped_conflicts;
    auto snapshot = scan_directory(rootA);
    auto result = build_state(skipped_conflicts, previous_state, snapshot, rootA);

    ASSERT_TRUE(result.has_value());
    previous_state = *result;

    {
        std::ofstream(rootA / "file.txt") << "NEW_A";
    }

    snapshot = scan_directory(rootA);
    skipped_conflicts.emplace("file.txt");
    result = build_state(skipped_conflicts, previous_state, snapshot, rootA);

    ASSERT_TRUE(result.has_value());
    const auto& state = *result;

    ASSERT_EQ(state.files.size(), 1);

    const auto& file = state.files.at("file.txt");

    ASSERT_EQ(file.size, 3);
    ASSERT_EQ(file.last_modified, previous_state.files.at("file.txt").last_modified);
    ASSERT_EQ(file.hash, previous_state.files.at("file.txt").hash);
}


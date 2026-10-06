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




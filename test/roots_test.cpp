#include <gtest/gtest.h>
#include <roots.h>
#include <fstream>

// Test 1
TEST(RootsTest, ResolveSimpleFilename) {
    const auto root = fs::temp_directory_path() / "filesync-roots-valid";

    fs::remove_all(root);
    fs::create_directories(root);

    {
        std::ofstream(root / "file.txt") << "file";
    }

    ASSERT_EQ(resolve_shared_path(root, "file.txt"), root / "file.txt");
    fs::remove_all(root);
}

// Test 2
TEST(RootsTest, ResolveNestedFilename) {
    const auto root = fs::temp_directory_path() / "filesync_roots-valid";

    fs::remove_all(root);
    fs::create_directories(root / "A");

    {
        std::ofstream(root / "A/file.txt") << "file";
    }

    ASSERT_EQ(resolve_shared_path(root, "A/file.txt"), fs::canonical(root / "A/file.txt"));
    fs::remove_all(root);
}

// Test 3
TEST(RootsTest, RejectAbsolutePath) {
    const auto root = fs::temp_directory_path() / "filesync-roots-invalid";

    fs::remove_all(root);
    fs::create_directories(root);

    {
        std::ofstream(root / "file.txt") << "file";
    }

    ASSERT_EQ(resolve_shared_path(root, root / "file.txt"), std::nullopt);
    fs::remove_all(root);
}

// Test 4
TEST(RootsTest, RejectDoubleDotPath) {
    const auto root = fs::temp_directory_path() / "filesync_roots-invalid";

    fs::remove_all(root);
    fs::create_directories(root / "A");

    {
        std::ofstream(root / "file.txt") << "file";
    }

    ASSERT_EQ(resolve_shared_path(root / "A", "../file.txt"), std::nullopt);
    fs::remove_all(root);
}

// Test 5
TEST(RootsTest, RejectMissingFile) {
    const auto root = fs::temp_directory_path() / "filesync-roots_invalid";

    fs::remove_all(root);
    fs::create_directories(root);

    ASSERT_EQ(resolve_shared_path(root, "file.txt"), std::nullopt);
    fs::remove_all(root);
}

// Test 6
TEST(RootsTest, RejectDirectory) {
    const auto root = fs::temp_directory_path() / "filesync_root";

    fs::remove_all(root);
    fs::create_directories(root / "A");

    ASSERT_EQ(resolve_shared_path(root, "A"), std::nullopt);
    fs::remove_all(root);
}

// Test 7
TEST(RootsTest, RejectSymlinkPointingOutsideRoot) {
    const auto root = fs::temp_directory_path() / "filesync-root";

    fs::remove_all(root);
    fs::create_directories(root / "A");

    fs::path target = root / "target.txt";
    fs::path link = root / "A/link.txt";
    std::ofstream(target) << "hello";
    
    std::error_code ec;
    fs::create_symlink(target, link, ec);
    ASSERT_FALSE(ec);

    ASSERT_EQ(resolve_shared_path(root / "A", "link.txt"), std::nullopt);
    fs::remove_all(root);
}
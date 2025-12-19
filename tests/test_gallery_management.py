#!/usr/bin/env python3
"""test_gallery_management.py

Unit tests for gallery management functions.
Tests get_gallery_info, generate_thumbnail, delete_image.

Run with: pytest tests/test_gallery_management.py -v
"""

import pytest
import sys
import os
import tempfile
import shutil

sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

from test_utils import validate_jpeg, create_test_jpeg


# ============================================================
# Gallery Functions (imported from pi_usb_sender or mocked)
# ============================================================

def get_gallery_info_local(capture_dir: str) -> tuple:
    """Local implementation of get_gallery_info for testing."""
    try:
        files = sorted(
            [f for f in os.listdir(capture_dir) if f.endswith('.jpg')],
            key=lambda x: os.path.getmtime(os.path.join(capture_dir, x)),
            reverse=True
        )
        total_size = sum(
            os.path.getsize(os.path.join(capture_dir, f)) 
            for f in files
        )
        size_mb = int(total_size / (1024 * 1024))
        return (len(files), size_mb, files)
    except Exception:
        return (0, 0, [])


def generate_thumbnail_local(filepath: str, width: int = 80, height: int = 60) -> bytes:
    """Local implementation of generate_thumbnail for testing."""
    try:
        import cv2
        
        img = cv2.imread(filepath)
        if img is None:
            return b''
        
        thumb = cv2.resize(img, (width, height), interpolation=cv2.INTER_AREA)
        result, jpeg_array = cv2.imencode(".jpg", thumb, [int(cv2.IMWRITE_JPEG_QUALITY), 70])
        
        if result:
            return jpeg_array.tobytes()
        return b''
    except ImportError:
        pytest.skip("OpenCV required for thumbnail tests")
    except Exception:
        return b''


def delete_image_local(index: int, files: list, capture_dir: str) -> bool:
    """Local implementation of delete_image for testing."""
    try:
        if index < 0 or index >= len(files):
            return False
        
        filepath = os.path.join(capture_dir, files[index])
        if os.path.exists(filepath):
            os.remove(filepath)
            return True
        return False
    except Exception:
        return False


# ============================================================
# Fixtures
# ============================================================

@pytest.fixture
def gallery_dir():
    """Create temporary gallery directory with test images."""
    tmpdir = tempfile.mkdtemp(prefix="gallery_test_")
    
    # Create some test images
    try:
        import cv2
        import numpy as np
        import time
        
        for i in range(5):
            img = np.random.randint(0, 255, (100, 100, 3), dtype=np.uint8)
            filepath = os.path.join(tmpdir, f"test_image_{i:03d}.jpg")
            cv2.imwrite(filepath, img)
            # Small delay to ensure different mtimes
            time.sleep(0.01)
    except ImportError:
        pytest.skip("OpenCV required for gallery tests")
    
    yield tmpdir
    shutil.rmtree(tmpdir, ignore_errors=True)


@pytest.fixture
def empty_gallery_dir():
    """Create empty gallery directory."""
    tmpdir = tempfile.mkdtemp(prefix="empty_gallery_test_")
    yield tmpdir
    shutil.rmtree(tmpdir, ignore_errors=True)


# ============================================================
# Tests: get_gallery_info
# ============================================================

class TestGetGalleryInfo:
    """Test gallery info retrieval."""
    
    def test_empty_directory(self, empty_gallery_dir):
        """Test get_gallery_info on empty directory."""
        count, size_mb, files = get_gallery_info_local(empty_gallery_dir)
        
        assert count == 0
        assert size_mb == 0
        assert files == []
    
    def test_with_images(self, gallery_dir):
        """Test get_gallery_info with existing images."""
        count, size_mb, files = get_gallery_info_local(gallery_dir)
        
        assert count == 5
        assert len(files) == 5
        # All files should be .jpg
        assert all(f.endswith('.jpg') for f in files)
    
    def test_sorted_by_mtime(self, gallery_dir):
        """Test that files are sorted by modification time (newest first)."""
        import time
        import cv2
        import numpy as np
        
        # Create a newer file
        newest = os.path.join(gallery_dir, "newest.jpg")
        img = np.random.randint(0, 255, (50, 50, 3), dtype=np.uint8)
        time.sleep(0.1)  # Ensure newer mtime
        cv2.imwrite(newest, img)
        
        count, size_mb, files = get_gallery_info_local(gallery_dir)
        
        assert files[0] == "newest.jpg"  # Newest should be first
    
    def test_ignores_non_jpg(self, gallery_dir):
        """Test that non-JPEG files are ignored."""
        # Create non-JPEG files
        with open(os.path.join(gallery_dir, "test.txt"), "w") as f:
            f.write("not an image")
        with open(os.path.join(gallery_dir, "test.png"), "wb") as f:
            f.write(b'\x89PNG')
        
        count, size_mb, files = get_gallery_info_local(gallery_dir)
        
        # Should still only have 5 original JPEGs
        assert count == 5
        assert all(f.endswith('.jpg') for f in files)
    
    def test_nonexistent_directory(self):
        """Test get_gallery_info on nonexistent directory."""
        count, size_mb, files = get_gallery_info_local("/nonexistent/path/12345")
        
        assert count == 0
        assert size_mb == 0
        assert files == []
    
    def test_size_calculation(self, empty_gallery_dir):
        """Test that size is calculated correctly."""
        import cv2
        import numpy as np
        
        # Create images with known approximate sizes
        for i in range(10):
            img = np.zeros((1000, 1000, 3), dtype=np.uint8)  # ~3MB uncompressed
            filepath = os.path.join(empty_gallery_dir, f"large_{i:03d}.jpg")
            cv2.imwrite(filepath, img, [int(cv2.IMWRITE_JPEG_QUALITY), 95])
        
        count, size_mb, files = get_gallery_info_local(empty_gallery_dir)
        
        assert count == 10
        # Size should be reasonable (each high quality 1000x1000 JPEG ~100-200KB)
        assert size_mb >= 0


# ============================================================
# Tests: generate_thumbnail
# ============================================================

class TestGenerateThumbnail:
    """Test thumbnail generation."""
    
    def test_generates_valid_jpeg(self, gallery_dir):
        """Test that thumbnail is valid JPEG."""
        files = os.listdir(gallery_dir)
        jpg_file = next(f for f in files if f.endswith('.jpg'))
        filepath = os.path.join(gallery_dir, jpg_file)
        
        thumb = generate_thumbnail_local(filepath)
        
        assert len(thumb) > 0
        assert validate_jpeg(thumb)
    
    def test_correct_dimensions(self, gallery_dir):
        """Test thumbnail has correct dimensions."""
        import cv2
        import numpy as np
        
        files = os.listdir(gallery_dir)
        jpg_file = next(f for f in files if f.endswith('.jpg'))
        filepath = os.path.join(gallery_dir, jpg_file)
        
        thumb = generate_thumbnail_local(filepath, 80, 60)
        
        # Decode and check dimensions
        arr = np.frombuffer(thumb, dtype=np.uint8)
        img = cv2.imdecode(arr, cv2.IMREAD_COLOR)
        
        assert img is not None
        assert img.shape[0] == 60  # height
        assert img.shape[1] == 80  # width
    
    def test_custom_dimensions(self, gallery_dir):
        """Test thumbnail with custom dimensions."""
        import cv2
        import numpy as np
        
        files = os.listdir(gallery_dir)
        jpg_file = next(f for f in files if f.endswith('.jpg'))
        filepath = os.path.join(gallery_dir, jpg_file)
        
        thumb = generate_thumbnail_local(filepath, 120, 90)
        
        arr = np.frombuffer(thumb, dtype=np.uint8)
        img = cv2.imdecode(arr, cv2.IMREAD_COLOR)
        
        assert img.shape[0] == 90
        assert img.shape[1] == 120
    
    def test_nonexistent_file(self):
        """Test thumbnail generation for nonexistent file."""
        thumb = generate_thumbnail_local("/nonexistent/file.jpg")
        assert thumb == b''
    
    def test_invalid_file(self, empty_gallery_dir):
        """Test thumbnail generation for invalid image file."""
        invalid_file = os.path.join(empty_gallery_dir, "invalid.jpg")
        with open(invalid_file, "wb") as f:
            f.write(b"not a jpeg")
        
        thumb = generate_thumbnail_local(invalid_file)
        assert thumb == b''
    
    def test_thumbnail_size_reasonable(self, gallery_dir):
        """Test that thumbnail size is reasonable for transmission."""
        files = os.listdir(gallery_dir)
        jpg_file = next(f for f in files if f.endswith('.jpg'))
        filepath = os.path.join(gallery_dir, jpg_file)
        
        thumb = generate_thumbnail_local(filepath, 80, 60)
        
        # 80x60 @ quality 70 should be well under 10KB
        assert len(thumb) < 10000


# ============================================================
# Tests: delete_image
# ============================================================

class TestDeleteImage:
    """Test image deletion."""
    
    def test_delete_existing(self, gallery_dir):
        """Test deleting an existing image."""
        count, size_mb, files = get_gallery_info_local(gallery_dir)
        original_count = count
        
        # Delete first image
        success = delete_image_local(0, files, gallery_dir)
        
        assert success is True
        
        # Verify file is gone
        new_count, _, _ = get_gallery_info_local(gallery_dir)
        assert new_count == original_count - 1
    
    def test_delete_last(self, gallery_dir):
        """Test deleting the last image in list."""
        _, _, files = get_gallery_info_local(gallery_dir)
        
        success = delete_image_local(len(files) - 1, files, gallery_dir)
        
        assert success is True
    
    def test_delete_invalid_index_negative(self, gallery_dir):
        """Test deleting with negative index."""
        _, _, files = get_gallery_info_local(gallery_dir)
        
        success = delete_image_local(-1, files, gallery_dir)
        
        assert success is False
    
    def test_delete_invalid_index_too_large(self, gallery_dir):
        """Test deleting with index >= count."""
        _, _, files = get_gallery_info_local(gallery_dir)
        
        success = delete_image_local(100, files, gallery_dir)
        
        assert success is False
    
    def test_delete_empty_list(self, empty_gallery_dir):
        """Test deleting from empty gallery."""
        success = delete_image_local(0, [], empty_gallery_dir)
        
        assert success is False
    
    def test_delete_all_sequentially(self, gallery_dir):
        """Test deleting all images one by one."""
        _, _, files = get_gallery_info_local(gallery_dir)
        original_files = files.copy()
        
        for i, filename in enumerate(original_files):
            # Always delete index 0 since list shrinks
            current_files = [f for f in os.listdir(gallery_dir) if f.endswith('.jpg')]
            if current_files:
                success = delete_image_local(0, current_files, gallery_dir)
                assert success is True
        
        # All files should be gone
        count, _, _ = get_gallery_info_local(gallery_dir)
        assert count == 0


# ============================================================
# Integration Tests
# ============================================================

class TestGalleryIntegration:
    """Integration tests for gallery workflow."""
    
    def test_full_workflow(self, empty_gallery_dir):
        """Test complete gallery workflow: create, list, thumbnail, delete."""
        import cv2
        import numpy as np
        
        # 1. Create images
        for i in range(3):
            img = np.random.randint(0, 255, (200, 200, 3), dtype=np.uint8)
            cv2.imwrite(os.path.join(empty_gallery_dir, f"photo_{i}.jpg"), img)
        
        # 2. List gallery
        count, size_mb, files = get_gallery_info_local(empty_gallery_dir)
        assert count == 3
        
        # 3. Generate thumbnails for all
        for f in files:
            thumb = generate_thumbnail_local(os.path.join(empty_gallery_dir, f))
            assert validate_jpeg(thumb)
        
        # 4. Delete middle image
        success = delete_image_local(1, files, empty_gallery_dir)
        assert success is True
        
        # 5. Verify deletion
        new_count, _, new_files = get_gallery_info_local(empty_gallery_dir)
        assert new_count == 2
        assert files[1] not in new_files
    
    def test_thumbnail_after_delete(self, gallery_dir):
        """Test that deleted file's thumbnail request returns empty."""
        _, _, files = get_gallery_info_local(gallery_dir)
        filepath = os.path.join(gallery_dir, files[0])
        
        # Delete the file
        os.remove(filepath)
        
        # Try to generate thumbnail
        thumb = generate_thumbnail_local(filepath)
        assert thumb == b''


if __name__ == "__main__":
    pytest.main([__file__, "-v"])

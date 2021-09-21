import os
import sys
import shutil

class KrkrExtractCleanup(object):
    def __init__(self):
        super().__init__()
        
    @property
    def current_path(self)-> str:
        path = os.path.abspath(__file__)
        path = os.path.dirname(path)
        return path
        
    def _delete_folder(self, path: str):
        if not os.path.exists(path):
            return
        if not os.path.isdir(path):
            return
        
        print("deleting %s" % path)
        try:
            shutil.rmtree(path)
        except Exception as e:
            print("error (%s) occurred while deleting %s", str(e), path)
            
    def _delete_file(self, path: str):
        if not os.path.exists(path):
            return
        if not os.path.isfile(path):
            return
        
        print("deleting %s" % path)
        try:
            os.remove(path)
        except Exception as e:
            print("error (%s) occurred while deleting %s", str(e), path)
    
    def _clean_capstone_static(self):
        path = self.current_path
        path = os.path.join(path, "capstone_static")
        path = os.path.join(path, "Release")
        self._delete_folder(path)
        
    def _clean_jsoncpp_static(self):
        path = self.current_path
        path = os.path.join(path, "jsoncpp_static")
        path = os.path.join(path, "Release")
        self._delete_folder(path)
        
    def _clean_krkrextract_core(self):
        path = self.current_path
        path = os.path.join(path, "KrkrExtract.Core")
        path = os.path.join(path, "Release")
        self._delete_folder(path)
        
    def _clean_krkrextract_lite(self):
        path = self.current_path
        path = os.path.join(path, "KrkrExtract.Lite")
        path = os.path.join(path, "bin")
        self._delete_folder(path)
        
        path = self.current_path
        path = os.path.join(path, "KrkrExtract.Lite")
        path = os.path.join(path, "obj")
        self._delete_folder(path)
        
    def _clean_krkrextract_static_analysis(self):
        path = self.current_path
        path = os.path.join(path, "KrkrExtract.StaticAnalysis")
        path = os.path.join(path, "bin")
        self._delete_folder(path)
        
        path = self.current_path
        path = os.path.join(path, "KrkrExtract.StaticAnalysis")
        path = os.path.join(path, "obj")
        self._delete_folder(path)
        
    def _clean_krkrextract_ui_lite(self):
        path = self.current_path
        path = os.path.join(path, "KrkrExtract.UI.Lite")
        path = os.path.join(path, "Release")
        self._delete_folder(path)
    
    def _clean_krkrextract_universal_patch(self):
        path = self.current_path
        path = os.path.join(path, "KrkrzUniversalPatch")
        path = os.path.join(path, "Release")
        self._delete_folder(path)
        
    def _clean_krkrextract_libpng(self):
        path = self.current_path
        path = os.path.join(path, "libpng")
        path = os.path.join(path, "Release")
        self._delete_folder(path)
        
    def _clean_krkrextract_nativelib(self):
        path = self.current_path
        path = os.path.join(path, "NativeLib")
        path = os.path.join(path, "Release")
        self._delete_folder(path)
        
    def _clean_krkrextract_node_modules(self):
        path = self.current_path
        path = os.path.join(path, "node_modules")
        self._delete_folder(path)
        
    def _clean_krkrextract_packages(self):
        path = self.current_path
        path = os.path.join(path, "packages")
        self._delete_folder(path)
        
    def _clean_krkrextract_patchloader(self):
        path = self.current_path
        path = os.path.join(path, "PatchLoader")
        path = os.path.join(path, "Release")
        self._delete_folder(path)
        
    def _clean_krkrextract_release(self):
        path = self.current_path
        path = os.path.join(path, "Release")
        self._delete_folder(path)
        
    def _clean_krkrextract_zlib(self):
        path = self.current_path
        path = os.path.join(path, "zlib")
        path = os.path.join(path, "Release")
        self._delete_folder(path)
        
        
    def _clean_krkrextract_vs(self):
        path = self.current_path
        path = os.path.join(path, ".vs")
        self._delete_folder(path)
    
    
    def cleanup(self):
        self._clean_capstone_static()
        self._clean_jsoncpp_static()
        self._clean_krkrextract_core()
        self._clean_krkrextract_lite()
        self._clean_krkrextract_static_analysis()
        self._clean_krkrextract_ui_lite()
        self._clean_krkrextract_universal_patch()
        self._clean_krkrextract_libpng()
        self._clean_krkrextract_nativelib()
        self._clean_krkrextract_node_modules()
        self._clean_krkrextract_packages()
        self._clean_krkrextract_patchloader()
        self._clean_krkrextract_release()
        self._clean_krkrextract_zlib()
        self._clean_krkrextract_vs()
        
if __name__ == "__main__":
    kc = KrkrExtractCleanup()
    kc.cleanup()

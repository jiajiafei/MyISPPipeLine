from PIL import Image
import numpy as np
import os  # 新增：用于遍历目录、处理文件路径

def shift_pixels(image_path, output_path, up_shift=3, left_shift=1):
    """
    将图像像素向上、向左移动指定像素数
    
    参数:
        image_path: 输入JPG图像路径
        output_path: 输出图像路径
        up_shift: 向上移动的像素数（默认3）
        left_shift: 向左移动的像素数（默认1）
    """
    try:
        # 打开图像并转换为numpy数组
        img = Image.open(image_path)
        img_array = np.array(img)  # 形状为 (高度, 宽度, 通道数)
        
        # 1. 向上移动up_shift个像素
        if up_shift > 0:
            # 保留从up_shift行开始到最后一行的所有像素
            img_array = img_array[up_shift:, :, :]
            # 计算需要补充的行数，并创建全黑行（0值）
            pad_rows = np.zeros((up_shift, img_array.shape[1], img_array.shape[2]), dtype=np.uint8)
            # 将黑行添加到图像底部
            img_array = np.vstack((img_array, pad_rows))
        
        # 2. 向左移动left_shift个像素
        if left_shift > 0:
            # 保留从left_shift列开始到最后一列的所有像素
            img_array = img_array[:, left_shift:, :]
            # 计算需要补充的列数，并创建全黑列（0值）
            pad_cols = np.zeros((img_array.shape[0], left_shift, img_array.shape[2]), dtype=np.uint8)
            # 将黑列添加到图像右侧
            img_array = np.hstack((img_array, pad_cols))
        
        # 将处理后的数组转回图像并保存
        shifted_img = Image.fromarray(img_array)
        shifted_img.save(output_path)
        print(f"? 图像处理完成：{output_path}")
        
    except FileNotFoundError:
        print(f"? 错误：找不到文件 {image_path}")
    except Exception as e:
        print(f"? 处理 {image_path} 出错：{str(e)}")

def batch_process_images(folder_path, up_shift=3, left_shift=1):
    """
    批量处理指定目录下的所有JPG图片
    
    参数:
        folder_path: 存放JPG图片的目录路径（相对/绝对路径）
        up_shift: 向上移动像素数（默认3）
        left_shift: 向左移动像素数（默认1）
    """
    # 检查目录是否存在
    if not os.path.isdir(folder_path):
        print(f"? 错误：目录 {folder_path} 不存在！")
        return
    
    # 遍历目录下的所有文件
    for filename in os.listdir(folder_path):
        # 只处理JPG文件（区分大小写，如需兼容JPEG可添加 '.jpeg'）
        if filename.lower().endswith('.jpg'):
            # 拼接输入文件的完整路径
            input_path = os.path.join(folder_path, filename)
            # 构造输出文件名：原文件名（去掉后缀） + output + .jpg
            name_without_ext = os.path.splitext(filename)[0]  # 提取无后缀的文件名
            output_filename = f"{name_without_ext}.jpg"
            output_path = os.path.join(folder_path, output_filename)
            
            # 调用处理函数
            shift_pixels(input_path, output_path, up_shift, left_shift)
    
    print("\n? 批量处理完成！")

# 示例调用
if __name__ == "__main__":
    # 替换为你的图片目录路径（相对路径/绝对路径都可以）
    # 比如：相对路径 "images"，绝对路径 "D:\\my_photos\\test"
    target_folder = "."  # "." 代表当前目录，可自行修改
    # 批量处理目录下所有JPG
    batch_process_images(
        folder_path=target_folder,
        up_shift=3,    # 可调整向上移动像素数
        left_shift=1   # 可调整向左移动像素数
    )
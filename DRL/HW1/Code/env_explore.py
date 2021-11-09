import tqdm
import hydra
import os

import torch
import torch.nn as nn
import torch.nn.functional as F
import torch.optim as optim

import matplotlib.pyplot as plt
import torchvision.transforms as T

import numpy as np
import gym
import pybulletgym  # register PyBullet enviroments with open ai gym
from PIL import Image
from reacher_env import ReacherDaggerEnv
from utils import weight_init, ExpertBuffer


env=ReacherDaggerEnv(frame_height=120, frame_width=160)

obs=env.reset()
train_transforms = T.Compose([
    T.RandomResizedCrop(size=(60, 80), scale=(0.95, 1.0)),
])
eval_transforms = T.Compose([
    T.Resize(size=(60, 80))
])

obs = torch.from_numpy(obs).float().to("cpu").unsqueeze(0)

x=np.transpose(train_transforms(obs).squeeze().numpy(), (1,2,0))
x=Image.fromarray(x.astype(np.uint8))
print(x)
# env=gym.make("ReacherPyBulletEnv-v0")
# print(env.metadata)
# env.render()
# env.reset()
# for _ in range(10000):
#     env.render()
#     env.step(env.action_space.sample())
# env.close()


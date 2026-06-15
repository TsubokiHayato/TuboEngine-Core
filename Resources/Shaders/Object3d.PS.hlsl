#include"Object3d.hlsli"

struct Material
{
    float4 color; //マテリアルの色
    int enableLighting; //ライティングを有効にするかどうか
    float4x4 uvTransform; //UV変換行列
    float shininess; //反射の強さ
    float environmentCoefficient; // 環境マップ寄与度 [0:無し, 1:全反射]

};


struct DirectionalLight
{
    float4 color; //光の色
    float3 direction; //光の方向
    float intensity; //光の強さ
};

struct PointLight
{
    float4 color; //光の色
    float3 position; //光の位置
    float intensity; //光の強さ
};

struct SpotLight
{
    float4 color; //光の色
    float3 position; //光の位置
    float intensity; //光の強さ
    float3 direction; //光の方向
    float distance; //光の影響範囲
    float decay; //減衰率
    float cosAngle; //光の角度
    
    
};

struct Camera
{
    float3 worldPosition;
};

struct LightType
{
    //0 : 平行光源
	//1 : Phong反射モデル
	//2 : Blinn-Phong反射モデル
	//3 : PointLight
	//4 : SpotLight
    //5 : 環境マッピング
    int type;
};


//コンスタントバッファの定義
//使用例 : ConstantBuffer<構造体> 変数名 : register(b0);
ConstantBuffer<Material> gMaterial : register(b0);
Texture2D<float4> gTexture : register(t0);
SamplerState gSampler : register(s0);
ConstantBuffer<DirectionalLight> gDirectionalLight : register(b1);

ConstantBuffer<Camera> gCamera : register(b2);
ConstantBuffer<LightType> gLightType : register(b3);
ConstantBuffer<PointLight> gPointLight : register(b4);
ConstantBuffer<SpotLight> gSpotLight : register(b5);

TextureCube<float4> gCubeTexture : register(t1);

struct PixcelShaderOutput
{
    float4 color : SV_TARGET0;
};


PixcelShaderOutput main(VertexShaderOutPut input)
{
   
    PixcelShaderOutput output;
    float4 tranceformedUV = mul(float4(input.texcoord, 0.0f, 1.0f), gMaterial.uvTransform);
    float4 textureColor = gTexture.Sample(gSampler, tranceformedUV.xy);
   
    float4 materialColor = gMaterial.color * input.color;
  
    if (gMaterial.enableLighting != 0)
    {
        //平行光源
        if (gLightType.type == 0)
        {
            float NdotL = dot(normalize(input.normal), -gDirectionalLight.direction);
            float cos = pow(NdotL * 0.5f + 0.5f, 2.0f);
            output.color.rgb = materialColor.rgb * textureColor.rgb * gDirectionalLight.color.rgb * cos * gDirectionalLight.intensity;
            output.color.a = materialColor.a * textureColor.a;
        }
        //PhongReflection
        else if (gLightType.type == 1)
        {
            float NdotL = dot(normalize(input.normal), normalize(-gDirectionalLight.direction));
            float cos = pow(NdotL * 0.5f + 0.5f, 2.0f);
            float3 toEye = normalize(gCamera.worldPosition - input.worldPosition);
            float3 reflectLight = reflect(normalize(gDirectionalLight.direction), normalize(input.normal));
        
            float RtoE = dot(reflectLight, toEye);
            float specularPow = pow(saturate(RtoE), gMaterial.shininess);
        
            float3 diffuse = materialColor.rgb * textureColor.rgb * gDirectionalLight.color.rgb * cos * gDirectionalLight.intensity;
        
            float3 specular = gDirectionalLight.color.rgb * gDirectionalLight.intensity * specularPow * float3(1.0f, 1.0f, 1.0f);
        
            output.color.rgb = diffuse + specular;
            output.color.a = materialColor.a * textureColor.a;
        }
        //BlinnPhong
        else if (gLightType.type == 2)
        {
            
            float NdotL = dot(normalize(input.normal), normalize(-gDirectionalLight.direction));
            float cos = pow(NdotL * 0.5f + 0.5f, 2.0f);
            float3 toEye = normalize(gCamera.worldPosition - input.worldPosition);
            float3 reflectLight = reflect(normalize(gDirectionalLight.direction), normalize(input.normal));

            float3 halfVector = normalize(-gDirectionalLight.direction + toEye);
            float NDotH = dot(normalize(input.normal), halfVector);
            float specularPow = pow(saturate(NDotH), gMaterial.shininess);

            float3 diffuse = materialColor.rgb * textureColor.rgb * gDirectionalLight.color.rgb * cos * gDirectionalLight.intensity;
            float3 specular = gDirectionalLight.color.rgb * gDirectionalLight.intensity * specularPow * float3(1.0f, 1.0f, 1.0f);

            output.color.rgb = diffuse + specular;
            output.color.a = materialColor.a * textureColor.a;
      
        }
        //PointLight
        else if (gLightType.type == 3)
        {
    // Point Light
            float3 pointLightDirection = normalize(input.worldPosition - gPointLight.position);
            float NdotLPoint = dot(normalize(-input.normal), pointLightDirection);
            float cosPoint = pow(NdotLPoint * 0.5f + 0.5f, 2.0f);
            float3 toEye = normalize(gCamera.worldPosition - input.worldPosition);
            float3 reflectPointLight = reflect(pointLightDirection, normalize(input.normal));
            float RtoEPoint = dot(reflectPointLight, toEye);
            float specularPowPoint = pow(saturate(RtoEPoint), gMaterial.shininess);

            float3 diffusePointLight = materialColor.rgb * textureColor.rgb * gPointLight.color.rgb * cosPoint * gPointLight.intensity;
            float3 specularPointLight = gPointLight.color.rgb * gPointLight.intensity * specularPowPoint * float3(1.0f, 1.0f, 1.0f);

    // Combine lights
            output.color.rgb = diffusePointLight + specularPointLight;
            output.color.a = materialColor.a * textureColor.a;
        }
        //SpotLight
        else if (gLightType.type == 4)
        {
            
            float3 spotLightDirectionOnSurface = normalize(input.worldPosition - gSpotLight.position);
            float distance = length(input.worldPosition - gSpotLight.position);
            float attenuationFactor = 1.0f / (1.0f + gSpotLight.decay * distance * distance);
            float cosAngle = dot(spotLightDirectionOnSurface, gSpotLight.direction);
            float falloffFactor = saturate((cosAngle - gSpotLight.cosAngle) / (1.0f - gSpotLight.cosAngle));
            
            float3 diffuseSpotLight = materialColor.rgb * textureColor.rgb * gSpotLight.color.rgb * gSpotLight.intensity * attenuationFactor * falloffFactor;
            float3 toEye = normalize(gCamera.worldPosition - input.worldPosition);
            float3 reflectSpotLight = reflect(spotLightDirectionOnSurface, normalize(input.normal));
            float RtoESpot = dot(reflectSpotLight, toEye);
            float specularPowSpot = pow(saturate(RtoESpot), gMaterial.shininess);
            float3 specularSpotLight = gSpotLight.color.rgb * gSpotLight.intensity * specularPowSpot * float3(1.0f, 1.0f, 1.0f) * attenuationFactor * falloffFactor;

            output.color.rgb = diffuseSpotLight + specularSpotLight;
            output.color.a = materialColor.a * textureColor.a;
        }
        // 環境マッピング
        else if (gLightType.type == 5)
        {
            float3 cameraTopPosition = normalize(input.worldPosition - gCamera.worldPosition);
            float3 reflectedVector = reflect(cameraTopPosition, normalize(input.normal));
            float4 environmentColor = gCubeTexture.Sample(gSampler, reflectedVector);

        // 通常のライティング
            float NdotL = dot(normalize(input.normal), -gDirectionalLight.direction);
            float cos = pow(NdotL * 0.5f + 0.5f, 2.0f);
            float3 baseColor = materialColor.rgb * textureColor.rgb * gDirectionalLight.color.rgb * cos * gDirectionalLight.intensity;

        // 環境マップ色とベースカラーをenvironmentCoefficientで線形補間
            output.color.rgb = lerp(baseColor, environmentColor.rgb, gMaterial.environmentCoefficient);
            output.color.a = materialColor.a * textureColor.a;
        }
        else
        {
            output.color = materialColor * textureColor;
        }

        
        
    }
    else
    {
        output.color = materialColor * textureColor;
    }
    
    
    // 半透明もしっかりブレンドするため、完全に透明な場合のみ破棄
    if (output.color.a == 0.0f)
    {
        discard;
    }
  
    
    return output;
};








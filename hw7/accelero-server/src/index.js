import * as THREE from 'three'
import { UnrealBloomPass } from 'three/examples/jsm/postprocessing/UnrealBloomPass'
import { EffectComposer } from 'three/examples/jsm/postprocessing/EffectComposer'
import { RenderPass } from 'three/examples/jsm/postprocessing/RenderPass'
import { ShaderPass } from 'three/examples/jsm/postprocessing/ShaderPass'
import { Manager } from 'socket.io-client'
import ss from 'socket.io-stream/socket.io-stream'
import bloomFrag from './shader/bloom.frag'
import bloomVert from './shader/bloom.vert'

const roleRadius = 5
const enemyRadius = 10
const beamLength = 10
const beamSpeed = 50
const enemySpeed = 100
const minBeamPeriod = 100
const minEnemyPeriod = 1000

const enemyEdgeMap = [...Array(enemyRadius + 1)].reduce(
  (map, _, x) => ({
    ...map,
    [x]: Math.sqrt(enemyRadius ** 2 - x ** 2),
  }),
  {},
)

const beams = new Map()
const enemies = new Map()

const sleep = (ms) => new Promise((o) => setTimeout(o, ms))

const control = (scene) => {
  const manager = new Manager('/', {})
  const socket = manager.socket('/accelero')
  socket.on('connect', () => {
    console.log('websocket connected')
  })

  const sphere = new THREE.Mesh(
    new THREE.SphereGeometry(roleRadius, 32, 32),
    new THREE.MeshBasicMaterial({ color: 0xe0e0e0 }),
  )
  scene.add(sphere)

  let zeroAccelero
  let lastData
  let lastBeamTs

  ss(socket).on('data', (stream) => {
    ;(async () => {
      while (true) {
        const rawData = stream.read()
        if (rawData) {
          try {
            const datas = rawData.toString('utf-8').split('}{')
            if (datas.length > 1) {
              datas[0] = `{${datas[datas.length - 1]}`
            }
            const data = JSON.parse(datas[0])
            if (!zeroAccelero) {
              zeroAccelero = data
            }
            if (lastData) {
              const deltaT = data.ts - lastData.ts
              const xRatio = ((data.x + lastData.x) / 2 - zeroAccelero.x) / 1024
              if (Math.abs(xRatio) > 0.02) {
                sphere.position.x += (xRatio * deltaT) / 5
              }
              const yRatio = ((data.y + lastData.y) / 2 - zeroAccelero.y) / 1024
              if (Math.abs(yRatio) > 0.02) {
                sphere.position.z -= (yRatio * deltaT) / 5
              }
            }
            // Button pressed, generate beam
            if (data.bp) {
              if (
                !lastData?.bp ||
                (lastData?.bp && data.ts - lastBeamTs > minBeamPeriod)
              ) {
                lastBeamTs = data.ts
                const beam = new THREE.Line(
                  new THREE.BufferGeometry().setFromPoints([
                    new THREE.Vector3(0, 0, 0),
                    new THREE.Vector3(0, 0, -beamLength),
                  ]),
                  new THREE.LineBasicMaterial({ color: 0xe0e0e0 }),
                )
                beam.position.set(
                  sphere.position.x,
                  sphere.position.y,
                  sphere.position.z - roleRadius,
                )
                beam.layers.set(1)
                beams.set(beam.uuid, beam)
                scene.add(beam)
              }
            }
            lastData = data
          } catch (error) {
            console.error(error)
          }
        }
        await sleep(30)
      }
    })()
  })
}

const enemeyGenerator = (scene) => {
  let cumulatedT = 0
  return (deltaT) => {
    cumulatedT += deltaT * 1000
    if (cumulatedT > minEnemyPeriod) {
      cumulatedT %= minEnemyPeriod

      const enemy = new THREE.Mesh(
        new THREE.SphereGeometry(enemyRadius, 32, 32),
        new THREE.MeshBasicMaterial({ color: 0x303030 }),
      )
      enemy.position.set(200 - 400 * Math.random(), 0, -500)
      enemies.set(enemy.uuid, enemy)
      scene.add(enemy)
    }
  }
}

const createRenderer = (scene, camera) => {
  // Renderer
  const renderer = new THREE.WebGLRenderer()
  renderer.setSize(window.innerWidth, window.innerHeight)
  document.body.appendChild(renderer.domElement)
  renderer.autoClear = false

  // Post processer
  const bloomComposer = new EffectComposer(renderer)
  const renderPass = new RenderPass(scene, camera)
  const bloomPass = new UnrealBloomPass(
    new THREE.Vector2(window.innerWidth, window.innerHeight),
    5,
    0,
    0,
  )
  bloomComposer.renderToScreen = false
  bloomComposer.addPass(renderPass)
  bloomComposer.addPass(bloomPass)

  const finalComposer = new EffectComposer(renderer)
  const shaderPass = new ShaderPass(
    new THREE.ShaderMaterial({
      uniforms: {
        baseTexture: { value: null },
        bloomTexture: { value: bloomComposer.renderTarget2.texture },
      },
      fragmentShader: bloomFrag,
      vertexShader: bloomVert,
      defines: {},
    }),
    'baseTexture',
  )
  shaderPass.needsSwap = true
  finalComposer.addPass(renderPass)
  finalComposer.addPass(shaderPass)

  return {
    render() {
      renderer.clear()

      camera.layers.set(1)
      bloomComposer.render()

      camera.layers.set(0)
      finalComposer.render()
    },
  }
}

const main = async () => {
  const scene = new THREE.Scene()

  // Camera
  const camera = new THREE.PerspectiveCamera(
    75,
    window.innerWidth / window.innerHeight,
    0.1,
    1000,
  )
  camera.position.set(0, 100, 100)
  camera.lookAt(0, 0, 0)

  // Helper
  scene.add(new THREE.GridHelper(1000, 100))

  // Generate enemies
  const generateEnemy = enemeyGenerator(scene)

  // Control by socket
  control(scene)

  // Start render
  const renderer = createRenderer(scene, camera)
  const clock = new THREE.Clock()
  const animate = () => {
    const deltaT = clock.getDelta()
    generateEnemy(deltaT)

    // Move beams
    beams.forEach((beam, uuid) => {
      beam.position.setZ(beam.position.z - beamSpeed * deltaT)
      if (beam.position.z < -500) {
        beams.delete(uuid)
        scene.remove(beam)
      }
    })

    // Move enemies
    enemies.forEach((enemy, uuid) => {
      enemy.position.setZ(enemy.position.z + enemySpeed * deltaT)
      if (enemy.position.z > 0) {
        enemies.delete(uuid)
        scene.remove(enemy)
        return
      }
      beams.forEach((beam, beamUuid) => {
        const { x, z } = beam.position
        const deltaZ = enemyEdgeMap[Math.round(Math.abs(x - enemy.position.x))]
        if (deltaZ && z < deltaZ + enemy.position.z) {
          enemies.delete(uuid)
          beams.delete(beamUuid)
          scene.remove(enemy, beam)
        }
      })
    })

    // Render
    renderer.render()

    // Next
    requestAnimationFrame(animate)
  }
  requestAnimationFrame(animate)
}

main().catch(console.error)
